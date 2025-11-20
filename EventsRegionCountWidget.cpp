#include "EventsRegionCountWidget.h"
#include "RegionCountManager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <algorithm>

EventsRegionCountWidget::EventsRegionCountWidget(QWidget* parent)
    : QWidget(parent),
      autoRefreshEnabled_(true) {
    setupUI();

    // Setup auto-refresh timer
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &EventsRegionCountWidget::updateDisplay);
    refreshTimer_->start(UPDATE_INTERVAL_MS);

    // Initial load
    loadRegionData();
}

EventsRegionCountWidget::~EventsRegionCountWidget() {
    if (refreshTimer_) {
        refreshTimer_->stop();
    }
}

void EventsRegionCountWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Title and status section
    QHBoxLayout* headerLayout = new QHBoxLayout();

    QLabel* titleLabel = new QLabel("<h3>Region Object Counting Statistics</h3>");
    headerLayout->addWidget(titleLabel);

    headerLayout->addStretch();

    updateTimeLabel_ = new QLabel("Last update: Never");
    updateTimeLabel_->setStyleSheet("QLabel { color: #666; font-size: 10pt; }");
    headerLayout->addWidget(updateTimeLabel_);

    mainLayout->addLayout(headerLayout);

    // Control buttons
    QGroupBox* controlGroup = new QGroupBox("Controls");
    QHBoxLayout* controlLayout = new QHBoxLayout();

    autoRefreshToggle_ = new QPushButton("⏸ Pause Auto-Refresh");
    autoRefreshToggle_->setCheckable(true);
    autoRefreshToggle_->setChecked(false);  // Not paused initially
    autoRefreshToggle_->setToolTip("Toggle automatic data refresh (500ms interval)");
    connect(autoRefreshToggle_, &QPushButton::toggled, this, &EventsRegionCountWidget::onAutoRefreshToggled);
    controlLayout->addWidget(autoRefreshToggle_);

    refreshButton_ = new QPushButton("🔄 Refresh Now");
    refreshButton_->setToolTip("Manually refresh region count data");
    connect(refreshButton_, &QPushButton::clicked, this, &EventsRegionCountWidget::onRefresh);
    controlLayout->addWidget(refreshButton_);

    exportButton_ = new QPushButton("💾 Export to JSON");
    exportButton_->setToolTip("Export current region count data to JSON file");
    connect(exportButton_, &QPushButton::clicked, this, &EventsRegionCountWidget::onExport);
    controlLayout->addWidget(exportButton_);

    clearRegionButton_ = new QPushButton("🗑 Clear Selected Region");
    clearRegionButton_->setToolTip("Clear count data for selected region");
    connect(clearRegionButton_, &QPushButton::clicked, this, &EventsRegionCountWidget::onClearRegion);
    controlLayout->addWidget(clearRegionButton_);

    clearAllButton_ = new QPushButton("⚠ Clear All Regions");
    clearAllButton_->setStyleSheet("QPushButton { background-color: #d9534f; color: white; }");
    clearAllButton_->setToolTip("Clear count data for ALL regions");
    connect(clearAllButton_, &QPushButton::clicked, this, &EventsRegionCountWidget::onClearAll);
    controlLayout->addWidget(clearAllButton_);

    controlGroup->setLayout(controlLayout);
    mainLayout->addWidget(controlGroup);

    // Table widget
    tableWidget_ = new QTableWidget(this);
    tableWidget_->setColumnCount(3);
    tableWidget_->setHorizontalHeaderLabels({"Region Name", "Unique Object Count", "Object IDs"});

    // Configure table appearance
    tableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget_->setAlternatingRowColors(true);
    tableWidget_->setSortingEnabled(true);
    tableWidget_->verticalHeader()->setVisible(false);

    // Column widths
    tableWidget_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tableWidget_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    tableWidget_->setColumnWidth(0, 200);

    // Style
    tableWidget_->setStyleSheet(
        "QTableWidget {"
        "    gridline-color: #d0d0d0;"
        "    background-color: white;"
        "}"
        "QTableWidget::item {"
        "    padding: 5px;"
        "}"
        "QHeaderView::section {"
        "    background-color: #f0f0f0;"
        "    padding: 8px;"
        "    border: 1px solid #d0d0d0;"
        "    font-weight: bold;"
        "}"
    );

    mainLayout->addWidget(tableWidget_);

    // Status bar
    QHBoxLayout* statusLayout = new QHBoxLayout();

    statusLabel_ = new QLabel("Total Regions: 0 | Total Objects Counted: 0");
    statusLabel_->setStyleSheet("QLabel { font-weight: bold; padding: 5px; }");
    statusLayout->addWidget(statusLabel_);

    statusLayout->addStretch();

    QLabel* infoLabel = new QLabel("ℹ Auto-refresh: 500ms | Click row to select");
    infoLabel->setStyleSheet("QLabel { color: #666; font-size: 9pt; }");
    statusLayout->addWidget(infoLabel);

    mainLayout->addLayout(statusLayout);

    setLayout(mainLayout);
}

void EventsRegionCountWidget::loadRegionData() {
    auto data = RegionCountManager::getInstance().getAllRegionData();
    populateTable(data);

    // Update last update time
    updateTimeLabel_->setText(
        QString("Last update: %1")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
    );
}

void EventsRegionCountWidget::populateTable(
    const std::map<std::string, std::pair<int, std::set<size_t>>>& data) {

    // Disable sorting temporarily for performance
    tableWidget_->setSortingEnabled(false);

    // Clear existing rows
    tableWidget_->setRowCount(0);

    int totalRegions = 0;
    int totalObjects = 0;

    int row = 0;
    for (const auto& [regionName, regionData] : data) {
        const int count = regionData.first;
        const std::set<size_t>& ids = regionData.second;

        tableWidget_->insertRow(row);

        // Region Name
        QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(regionName));
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tableWidget_->setItem(row, 0, nameItem);

        // Count
        QTableWidgetItem* countItem = new QTableWidgetItem(QString::number(count));
        countItem->setTextAlignment(Qt::AlignCenter);
        countItem->setData(Qt::UserRole, count);  // Store for sorting

        // Color code based on count
        if (count == 0) {
            countItem->setForeground(QBrush(QColor("#999999")));
        } else if (count < 5) {
            countItem->setForeground(QBrush(QColor("#5cb85c")));  // Green
        } else if (count < 10) {
            countItem->setForeground(QBrush(QColor("#f0ad4e")));  // Orange
        } else {
            countItem->setForeground(QBrush(QColor("#d9534f")));  // Red
        }

        tableWidget_->setItem(row, 1, countItem);

        // IDs list
        QString idsText = formatIdsList(ids);
        QTableWidgetItem* idsItem = new QTableWidgetItem(idsText);
        idsItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        idsItem->setToolTip(idsText);  // Show full list on hover
        tableWidget_->setItem(row, 2, idsItem);

        totalRegions++;
        totalObjects += count;
        row++;
    }

    // Re-enable sorting
    tableWidget_->setSortingEnabled(true);

    // Update status
    statusLabel_->setText(
        QString("Total Regions: %1 | Total Objects Counted: %2")
            .arg(totalRegions)
            .arg(totalObjects)
    );

    // Update clear button states
    clearRegionButton_->setEnabled(tableWidget_->rowCount() > 0);
    clearAllButton_->setEnabled(tableWidget_->rowCount() > 0);
    exportButton_->setEnabled(tableWidget_->rowCount() > 0);
}

QString EventsRegionCountWidget::formatIdsList(const std::set<size_t>& ids) const {
    if (ids.empty()) {
        return "(none)";
    }

    QString result;
    int count = 0;

    for (size_t id : ids) {
        if (count > 0) {
            result += ", ";
        }

        result += QString::number(id);
        count++;

        // Limit display to prevent UI slowdown
        if (count >= MAX_IDS_DISPLAY) {
            int remaining = ids.size() - MAX_IDS_DISPLAY;
            if (remaining > 0) {
                result += QString(" ... (+%1 more)").arg(remaining);
            }
            break;
        }
    }

    return result;
}

void EventsRegionCountWidget::onRefresh() {
    loadRegionData();
}

void EventsRegionCountWidget::onExport() {
    QString defaultFileName = QString("region_count_%1.json")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Export Region Count Data",
        defaultFileName,
        "JSON Files (*.json);;All Files (*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    bool success = RegionCountManager::getInstance().saveToJson(fileName.toStdString());

    if (success) {
        QMessageBox::information(
            this,
            "Export Successful",
            QString("Region count data exported to:\n%1").arg(fileName)
        );
    } else {
        QMessageBox::critical(
            this,
            "Export Failed",
            QString("Failed to export data to:\n%1").arg(fileName)
        );
    }
}

void EventsRegionCountWidget::onClearAll() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear All Region Counts",
        "Are you sure you want to clear counting data for ALL regions?\n\n"
        "This will reset all counters to 0 and clear all tracked IDs.\n"
        "This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        RegionCountManager::getInstance().clearAll();
        loadRegionData();

        QMessageBox::information(
            this,
            "Cleared",
            "All region counting data has been cleared."
        );
    }
}

void EventsRegionCountWidget::onClearRegion() {
    int currentRow = tableWidget_->currentRow();

    if (currentRow < 0) {
        QMessageBox::warning(
            this,
            "No Selection",
            "Please select a region row to clear."
        );
        return;
    }

    QString regionName = tableWidget_->item(currentRow, 0)->text();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear Region Count",
        QString("Are you sure you want to clear counting data for region:\n\n%1\n\n"
                "This will reset the counter to 0 and clear all tracked IDs for this region.\n"
                "This action cannot be undone.").arg(regionName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        RegionCountManager::getInstance().clearRegion(regionName.toStdString());
        loadRegionData();

        QMessageBox::information(
            this,
            "Cleared",
            QString("Region '%1' counting data has been cleared.").arg(regionName)
        );
    }
}

void EventsRegionCountWidget::onAutoRefreshToggled(bool paused) {
    autoRefreshEnabled_ = !paused;

    if (autoRefreshEnabled_) {
        refreshTimer_->start(UPDATE_INTERVAL_MS);
        autoRefreshToggle_->setText("⏸ Pause Auto-Refresh");
        autoRefreshToggle_->setStyleSheet("");
    } else {
        refreshTimer_->stop();
        autoRefreshToggle_->setText("▶ Resume Auto-Refresh");
        autoRefreshToggle_->setStyleSheet("QPushButton { background-color: #f0ad4e; }");
    }
}

void EventsRegionCountWidget::updateDisplay() {
    if (autoRefreshEnabled_) {
        loadRegionData();
    }
}
