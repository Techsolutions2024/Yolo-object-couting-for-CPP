#include "ClassSelectionDialog.h"
#include <QMessageBox>
#include <QGroupBox>

ClassSelectionDialog::ClassSelectionDialog(const std::vector<std::string>& allClasses,
                                         const std::set<int>& currentSelection,
                                         QWidget* parent)
    : QDialog(parent),
      allClasses_(allClasses),
      selectedClasses_(currentSelection),
      countAllMode_(currentSelection.empty()) {

    setWindowTitle("Model Classes - Select Classes to Count");
    setMinimumSize(700, 600);

    setupUI();
    populateTable();
}

void ClassSelectionDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Title and info
    QLabel* titleLabel = new QLabel("Select Classes to Count");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    QLabel* infoLabel = new QLabel(
        "Choose which object classes to count in detections.\n"
        "If no classes are selected, all classes will be counted by default."
    );
    infoLabel->setStyleSheet("QLabel { color: #666; padding: 5px; }");
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);

    mainLayout->addSpacing(10);

    // Search box
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel("Search:");
    searchLineEdit_ = new QLineEdit();
    searchLineEdit_->setPlaceholderText("Type to search class name...");
    connect(searchLineEdit_, &QLineEdit::textChanged,
            this, &ClassSelectionDialog::onSearchTextChanged);

    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchLineEdit_);
    mainLayout->addLayout(searchLayout);

    // Table widget
    tableWidget_ = new QTableWidget();
    tableWidget_->setColumnCount(3);
    tableWidget_->setHorizontalHeaderLabels({"Class ID", "Class Name", "Selected"});

    // Configure table
    tableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget_->setSortingEnabled(true);
    tableWidget_->setAlternatingRowColors(true);

    // Column widths
    tableWidget_->horizontalHeader()->setSectionResizeMode(COL_ID, QHeaderView::ResizeToContents);
    tableWidget_->horizontalHeader()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    tableWidget_->horizontalHeader()->setSectionResizeMode(COL_CHECKBOX, QHeaderView::ResizeToContents);

    mainLayout->addWidget(tableWidget_);

    // Status label
    statusLabel_ = new QLabel();
    statusLabel_->setStyleSheet("QLabel { padding: 5px; font-weight: bold; }");
    mainLayout->addWidget(statusLabel_);

    // Bulk selection buttons
    QHBoxLayout* bulkLayout = new QHBoxLayout();
    selectAllButton_ = new QPushButton("Select All");
    deselectAllButton_ = new QPushButton("Deselect All");

    connect(selectAllButton_, &QPushButton::clicked, this, &ClassSelectionDialog::onSelectAll);
    connect(deselectAllButton_, &QPushButton::clicked, this, &ClassSelectionDialog::onDeselectAll);

    bulkLayout->addWidget(selectAllButton_);
    bulkLayout->addWidget(deselectAllButton_);
    bulkLayout->addStretch();
    mainLayout->addLayout(bulkLayout);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    cancelButton_ = new QPushButton("Cancel");
    applyButton_ = new QPushButton("Apply");
    applyButton_->setDefault(true);

    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    connect(applyButton_, &QPushButton::clicked, this, &ClassSelectionDialog::onApply);

    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(applyButton_);
    mainLayout->addLayout(buttonLayout);

    updateStatusLabel();
}

void ClassSelectionDialog::populateTable() {
    tableWidget_->setRowCount(static_cast<int>(allClasses_.size()));

    for (size_t i = 0; i < allClasses_.size(); ++i) {
        // Class ID
        QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(i));
        idItem->setTextAlignment(Qt::AlignCenter);
        tableWidget_->setItem(static_cast<int>(i), COL_ID, idItem);

        // Class Name
        QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(allClasses_[i]));
        tableWidget_->setItem(static_cast<int>(i), COL_NAME, nameItem);

        // Checkbox
        QCheckBox* checkBox = new QCheckBox();
        checkBox->setChecked(selectedClasses_.find(static_cast<int>(i)) != selectedClasses_.end());

        // Center the checkbox
        QWidget* checkBoxWidget = new QWidget();
        QHBoxLayout* checkBoxLayout = new QHBoxLayout(checkBoxWidget);
        checkBoxLayout->addWidget(checkBox);
        checkBoxLayout->setAlignment(Qt::AlignCenter);
        checkBoxLayout->setContentsMargins(0, 0, 0, 0);

        tableWidget_->setCellWidget(static_cast<int>(i), COL_CHECKBOX, checkBoxWidget);

        // Connect checkbox signal
        connect(checkBox, &QCheckBox::stateChanged, this, [this]() {
            updateStatusLabel();
        });
    }

    updateStatusLabel();
}

void ClassSelectionDialog::filterTable(const QString& searchText) {
    for (int row = 0; row < tableWidget_->rowCount(); ++row) {
        QTableWidgetItem* nameItem = tableWidget_->item(row, COL_NAME);

        if (nameItem) {
            QString className = nameItem->text();
            bool matches = className.contains(searchText, Qt::CaseInsensitive);
            tableWidget_->setRowHidden(row, !matches);
        }
    }
}

void ClassSelectionDialog::onSearchTextChanged(const QString& text) {
    filterTable(text);
}

void ClassSelectionDialog::onSelectAll() {
    for (int row = 0; row < tableWidget_->rowCount(); ++row) {
        if (!tableWidget_->isRowHidden(row)) {
            QWidget* widget = tableWidget_->cellWidget(row, COL_CHECKBOX);
            if (widget) {
                QCheckBox* checkBox = widget->findChild<QCheckBox*>();
                if (checkBox) {
                    checkBox->setChecked(true);
                }
            }
        }
    }
    updateStatusLabel();
}

void ClassSelectionDialog::onDeselectAll() {
    for (int row = 0; row < tableWidget_->rowCount(); ++row) {
        if (!tableWidget_->isRowHidden(row)) {
            QWidget* widget = tableWidget_->cellWidget(row, COL_CHECKBOX);
            if (widget) {
                QCheckBox* checkBox = widget->findChild<QCheckBox*>();
                if (checkBox) {
                    checkBox->setChecked(false);
                }
            }
        }
    }
    updateStatusLabel();
}

void ClassSelectionDialog::onApply() {
    // Collect selected classes
    selectedClasses_.clear();

    for (int row = 0; row < tableWidget_->rowCount(); ++row) {
        QWidget* widget = tableWidget_->cellWidget(row, COL_CHECKBOX);
        if (widget) {
            QCheckBox* checkBox = widget->findChild<QCheckBox*>();
            if (checkBox && checkBox->isChecked()) {
                QTableWidgetItem* idItem = tableWidget_->item(row, COL_ID);
                if (idItem) {
                    int classId = idItem->text().toInt();
                    selectedClasses_.insert(classId);
                }
            }
        }
    }

    countAllMode_ = selectedClasses_.empty();

    QString message;
    if (countAllMode_) {
        message = "No classes selected. Will count ALL classes by default.";
    } else {
        message = QString("Selected %1 class(es) to count.").arg(selectedClasses_.size());
    }

    QMessageBox::information(this, "Classes Updated", message);
    accept();
}

void ClassSelectionDialog::updateStatusLabel() {
    int selectedCount = 0;

    for (int row = 0; row < tableWidget_->rowCount(); ++row) {
        QWidget* widget = tableWidget_->cellWidget(row, COL_CHECKBOX);
        if (widget) {
            QCheckBox* checkBox = widget->findChild<QCheckBox*>();
            if (checkBox && checkBox->isChecked()) {
                selectedCount++;
            }
        }
    }

    if (selectedCount == 0) {
        statusLabel_->setText("⚠️  No classes selected - Will count ALL classes");
        statusLabel_->setStyleSheet("QLabel { color: orange; padding: 5px; font-weight: bold; }");
    } else if (selectedCount == static_cast<int>(allClasses_.size())) {
        statusLabel_->setText(QString("✅ All %1 classes selected").arg(selectedCount));
        statusLabel_->setStyleSheet("QLabel { color: green; padding: 5px; font-weight: bold; }");
    } else {
        statusLabel_->setText(QString("✅ %1 of %2 classes selected")
                             .arg(selectedCount)
                             .arg(allClasses_.size()));
        statusLabel_->setStyleSheet("QLabel { color: blue; padding: 5px; font-weight: bold; }");
    }
}

void ClassSelectionDialog::onHeaderCheckboxToggled(bool checked) {
    if (checked) {
        onSelectAll();
    } else {
        onDeselectAll();
    }
}

std::set<int> ClassSelectionDialog::getSelectedClasses() const {
    return selectedClasses_;
}
