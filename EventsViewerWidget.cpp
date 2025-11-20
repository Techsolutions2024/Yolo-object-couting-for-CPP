#include "EventsViewerWidget.h"
#include <QMessageBox>
#include <QGroupBox>
#include <set>

EventsViewerWidget::EventsViewerWidget(QWidget* parent)
    : QDialog(parent),
      filterActive_(false),
      currentPage_(0),
      totalPages_(0),
      currentColumns_(5) {  // Default 5 columns
    setupUI();
    loadEvents();
}

void EventsViewerWidget::setupUI() {
    setWindowTitle("Events & Region Count Viewer");
    setMinimumSize(1000, 700);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create tab widget
    tabWidget_ = new QTabWidget(this);

    // Create Events tab
    QWidget* eventsTab = new QWidget();
    setupEventsTab(eventsTab);
    tabWidget_->addTab(eventsTab, "📸 Events");

    // Create Region Count tab
    regionCountWidget_ = new EventsRegionCountWidget(this);
    tabWidget_->addTab(regionCountWidget_, "📊 Region Count");

    mainLayout->addWidget(tabWidget_);

    // Close button at bottom
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    closeButton_ = new QPushButton("Close");
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeButton_);

    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);
}

void EventsViewerWidget::setupEventsTab(QWidget* eventsTab) {
    QVBoxLayout* mainLayout = new QVBoxLayout(eventsTab);

    // Filter section
    QGroupBox* filterGroup = new QGroupBox("Filters");
    QHBoxLayout* filterLayout = new QHBoxLayout();

    // Camera filter
    QLabel* cameraLabel = new QLabel("Camera:");
    cameraFilterCombo_ = new QComboBox();
    cameraFilterCombo_->addItem("All Cameras", -1);
    filterLayout->addWidget(cameraLabel);
    filterLayout->addWidget(cameraFilterCombo_);

    // Date range
    QLabel* fromLabel = new QLabel("From:");
    startDateEdit_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-7));
    startDateEdit_->setCalendarPopup(true);
    startDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    filterLayout->addWidget(fromLabel);
    filterLayout->addWidget(startDateEdit_);

    QLabel* toLabel = new QLabel("To:");
    endDateEdit_ = new QDateTimeEdit(QDateTime::currentDateTime());
    endDateEdit_->setCalendarPopup(true);
    endDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    filterLayout->addWidget(toLabel);
    filterLayout->addWidget(endDateEdit_);

    // Filter buttons
    applyFilterButton_ = new QPushButton("Apply Filter");
    connect(applyFilterButton_, &QPushButton::clicked, this, &EventsViewerWidget::onApplyFilter);
    filterLayout->addWidget(applyFilterButton_);

    clearFilterButton_ = new QPushButton("Clear Filter");
    connect(clearFilterButton_, &QPushButton::clicked, this, &EventsViewerWidget::onClearFilter);
    filterLayout->addWidget(clearFilterButton_);

    filterGroup->setLayout(filterLayout);
    mainLayout->addWidget(filterGroup);

    // Events grid
    scrollArea_ = new QScrollArea();
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    gridContainer_ = new QWidget();
    gridLayout_ = new QGridLayout(gridContainer_);
    gridLayout_->setSpacing(10);
    gridLayout_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    scrollArea_->setWidget(gridContainer_);
    mainLayout->addWidget(scrollArea_);

    // NEW: Pagination controls
    QHBoxLayout* paginationLayout = new QHBoxLayout();

    prevPageButton_ = new QPushButton("◀ Previous");
    connect(prevPageButton_, &QPushButton::clicked, this, &EventsViewerWidget::onPreviousPage);
    paginationLayout->addWidget(prevPageButton_);

    pageInfoLabel_ = new QLabel("Page 1 / 1");
    pageInfoLabel_->setAlignment(Qt::AlignCenter);
    pageInfoLabel_->setStyleSheet("QLabel { font-weight: bold; padding: 5px; }");
    paginationLayout->addWidget(pageInfoLabel_);

    QLabel* goToLabel = new QLabel("Go to page:");
    paginationLayout->addWidget(goToLabel);

    pageSpinBox_ = new QSpinBox();
    pageSpinBox_->setMinimum(1);
    pageSpinBox_->setMaximum(1);
    pageSpinBox_->setValue(1);
    connect(pageSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &EventsViewerWidget::onPageChanged);
    paginationLayout->addWidget(pageSpinBox_);

    nextPageButton_ = new QPushButton("Next ▶");
    connect(nextPageButton_, &QPushButton::clicked, this, &EventsViewerWidget::onNextPage);
    paginationLayout->addWidget(nextPageButton_);

    paginationLayout->addStretch();

    mainLayout->addLayout(paginationLayout);

    // Status and action buttons
    QHBoxLayout* bottomLayout = new QHBoxLayout();

    statusLabel_ = new QLabel("Total Events: 0");
    bottomLayout->addWidget(statusLabel_);

    bottomLayout->addStretch();

    refreshButton_ = new QPushButton("Refresh");
    connect(refreshButton_, &QPushButton::clicked, this, &EventsViewerWidget::onRefresh);
    bottomLayout->addWidget(refreshButton_);

    clearAllButton_ = new QPushButton("Clear All Events");
    clearAllButton_->setStyleSheet("QPushButton { background-color: #d9534f; color: white; }");
    connect(clearAllButton_, &QPushButton::clicked, this, &EventsViewerWidget::onClearAllEvents);
    bottomLayout->addWidget(clearAllButton_);

    mainLayout->addLayout(bottomLayout);

    eventsTab->setLayout(mainLayout);
}

void EventsViewerWidget::loadEvents() {
    // Get all events
    EventManager& manager = EventManager::getInstance();
    allEvents_ = manager.getAllEvents();  // NEW: Store in member variable

    // Populate camera filter dropdown
    std::set<int> cameraIds;
    for (const auto& event : allEvents_) {
        cameraIds.insert(event.getCameraId());
    }

    cameraFilterCombo_->clear();
    cameraFilterCombo_->addItem("All Cameras", -1);
    for (int id : cameraIds) {
        if (!allEvents_.empty()) {
            // Find camera name
            for (const auto& event : allEvents_) {
                if (event.getCameraId() == id) {
                    cameraFilterCombo_->addItem(
                        QString::fromStdString(event.getCameraName()),
                        id
                    );
                    break;
                }
            }
        }
    }

    // NEW: Initialize pagination
    currentPage_ = 0;
    totalPages_ = allEvents_.empty() ? 1 :
                  (allEvents_.size() + EVENTS_PER_PAGE - 1) / EVENTS_PER_PAGE;

    // Display first page
    displayCurrentPage();
    updatePaginationControls();
}

void EventsViewerWidget::clearThumbnails() {
    for (auto* widget : thumbnailWidgets_) {
        gridLayout_->removeWidget(widget);
        widget->deleteLater();
    }
    thumbnailWidgets_.clear();
}

void EventsViewerWidget::displayEvents(const std::vector<DetectionEvent>& events) {
    clearThumbnails();

    int columns = calculateOptimalColumns();  // NEW: Dynamic columns
    currentColumns_ = columns;
    int row = 0, col = 0;

    for (const auto& event : events) {
        auto* thumbnail = new EventThumbnailWidget(event, this);
        connect(thumbnail, &EventThumbnailWidget::clicked,
                this, &EventsViewerWidget::onThumbnailClicked);

        gridLayout_->addWidget(thumbnail, row, col);
        thumbnailWidgets_.push_back(thumbnail);

        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    }

    // NOTE: Status is now updated in displayCurrentPage()
}

// NEW: Display current page of events
void EventsViewerWidget::displayCurrentPage() {
    if (allEvents_.empty()) {
        clearThumbnails();
        statusLabel_->setText("Total Events: 0");
        return;
    }

    int startIdx = currentPage_ * EVENTS_PER_PAGE;
    int endIdx = std::min(startIdx + EVENTS_PER_PAGE, static_cast<int>(allEvents_.size()));

    std::vector<DetectionEvent> pageEvents(
        allEvents_.begin() + startIdx,
        allEvents_.begin() + endIdx
    );

    displayEvents(pageEvents);

    // Update status with pagination info
    statusLabel_->setText(
        QString("Showing %1-%2 of %3 events (Page %4/%5)")
            .arg(startIdx + 1)
            .arg(endIdx)
            .arg(allEvents_.size())
            .arg(currentPage_ + 1)
            .arg(totalPages_)
    );
}

// NEW: Update pagination button states
void EventsViewerWidget::updatePaginationControls() {
    prevPageButton_->setEnabled(currentPage_ > 0);
    nextPageButton_->setEnabled(currentPage_ < totalPages_ - 1);

    pageInfoLabel_->setText(
        QString("Page %1 / %2")
            .arg(currentPage_ + 1)
            .arg(totalPages_)
    );

    // Update spinbox
    pageSpinBox_->blockSignals(true);
    pageSpinBox_->setMaximum(totalPages_);
    pageSpinBox_->setValue(currentPage_ + 1);
    pageSpinBox_->blockSignals(false);
}

// NEW: Calculate optimal column count based on width
int EventsViewerWidget::calculateOptimalColumns() const {
    static constexpr int THUMBNAIL_WIDTH = 160;
    static constexpr int MIN_COLUMNS = 3;
    static constexpr int MAX_COLUMNS = 10;

    int availableWidth = scrollArea_->viewport()->width();
    int columns = availableWidth / THUMBNAIL_WIDTH;
    return std::clamp(columns, MIN_COLUMNS, MAX_COLUMNS);
}

void EventsViewerWidget::onRefresh() {
    loadEvents();
}

void EventsViewerWidget::onApplyFilter() {
    EventManager& manager = EventManager::getInstance();

    // Get camera filter
    int selectedCameraId = cameraFilterCombo_->currentData().toInt();

    // Get time range
    QDateTime startTime = startDateEdit_->dateTime();
    QDateTime endTime = endDateEdit_->dateTime();

    // Apply filters
    if (selectedCameraId == -1) {
        // All cameras, just filter by time
        allEvents_ = manager.getEventsByTimeRange(startTime, endTime);
    } else {
        // Filter by camera first
        std::vector<DetectionEvent> cameraEvents = manager.getEventsByCamera(selectedCameraId);

        // Then filter by time
        allEvents_.clear();
        for (const auto& event : cameraEvents) {
            QDateTime eventTime = event.getTimestamp();
            if (eventTime >= startTime && eventTime <= endTime) {
                allEvents_.push_back(event);
            }
        }
    }

    filterActive_ = true;

    // NEW: Reset to page 0 after filtering
    currentPage_ = 0;
    totalPages_ = allEvents_.empty() ? 1 :
                  (allEvents_.size() + EVENTS_PER_PAGE - 1) / EVENTS_PER_PAGE;

    displayCurrentPage();
    updatePaginationControls();
}

void EventsViewerWidget::onClearFilter() {
    filterActive_ = false;
    cameraFilterCombo_->setCurrentIndex(0);
    startDateEdit_->setDateTime(QDateTime::currentDateTime().addDays(-7));
    endDateEdit_->setDateTime(QDateTime::currentDateTime());
    loadEvents();
}

void EventsViewerWidget::onClearAllEvents() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear All Events",
        "Are you sure you want to delete all events?\n"
        "This will clear the event list but NOT delete saved images.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        EventManager::getInstance().clearEvents();
        loadEvents();
    }
}

void EventsViewerWidget::onThumbnailClicked(const DetectionEvent& event) {
    // Show full-size image in a dialog
    QDialog imageDialog(this);
    imageDialog.setWindowTitle(
        QString::fromStdString(event.getCameraName() + " - " + event.getRegionName())
    );

    QVBoxLayout* layout = new QVBoxLayout(&imageDialog);

    // Load full image
    std::string imagePath = event.getImagePath();
    cv::Mat img = cv::imread(imagePath);

    if (!img.empty()) {
        cv::Mat rgb;
        cv::cvtColor(img, rgb, cv::COLOR_BGR2RGB);

        QImage qimg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
        QPixmap pixmap = QPixmap::fromImage(qimg.copy());

        QLabel* imageLabel = new QLabel();
        imageLabel->setPixmap(pixmap);
        imageLabel->setScaledContents(false);
        layout->addWidget(imageLabel);

        // Event details
        QString details = QString(
            "<b>Track ID:</b> %1 | <b>Class:</b> %2 | <b>Confidence:</b> %3% | "
            "<b>Type:</b> %4 | <b>Time:</b> %5"
        )
            .arg(event.getTrackId())
            .arg(QString::fromStdString(event.getObjectClass()))
            .arg(static_cast<int>(event.getConfidence() * 100))
            .arg(QString::fromStdString(event.getEventTypeString()))
            .arg(event.getTimestamp().toString("yyyy-MM-dd HH:mm:ss"));

        QLabel* detailsLabel = new QLabel(details);
        detailsLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(detailsLabel);

        imageDialog.resize(pixmap.width() + 40, pixmap.height() + 100);
    } else {
        QLabel* errorLabel = new QLabel("Image not found or could not be loaded.");
        layout->addWidget(errorLabel);
    }

    QPushButton* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, &imageDialog, &QDialog::accept);
    layout->addWidget(closeBtn);

    imageDialog.exec();
}

// NEW: Pagination slot handlers
void EventsViewerWidget::onPreviousPage() {
    if (currentPage_ > 0) {
        currentPage_--;
        displayCurrentPage();
        updatePaginationControls();
    }
}

void EventsViewerWidget::onNextPage() {
    if (currentPage_ < totalPages_ - 1) {
        currentPage_++;
        displayCurrentPage();
        updatePaginationControls();
    }
}

void EventsViewerWidget::onPageChanged() {
    int newPage = pageSpinBox_->value() - 1;  // Convert to 0-based index
    if (newPage >= 0 && newPage < totalPages_ && newPage != currentPage_) {
        currentPage_ = newPage;
        displayCurrentPage();
        updatePaginationControls();
    }
}
