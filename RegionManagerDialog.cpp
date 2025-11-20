#include "RegionManagerDialog.h"
#include <QInputDialog>
#include <QMessageBox>

RegionManagerDialog::RegionManagerDialog(std::vector<Region>& regions, QWidget* parent)
    : QDialog(parent), regions_(regions) {
    setupUI();
    populateList();
    updateButtonStates();
}

void RegionManagerDialog::setupUI() {
    setWindowTitle("Manage Regions");
    setMinimumWidth(400);
    setMinimumHeight(300);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Info label
    infoLabel_ = new QLabel(QString("Total Regions: %1").arg(regions_.size()));
    mainLayout->addWidget(infoLabel_);

    // List widget
    regionListWidget_ = new QListWidget();
    connect(regionListWidget_, &QListWidget::itemSelectionChanged,
            this, &RegionManagerDialog::updateButtonStates);
    mainLayout->addWidget(regionListWidget_);

    // Buttons layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    renameButton_ = new QPushButton("Rename");
    connect(renameButton_, &QPushButton::clicked, this, &RegionManagerDialog::onRenameClicked);
    buttonLayout->addWidget(renameButton_);

    deleteButton_ = new QPushButton("Delete");
    deleteButton_->setStyleSheet("QPushButton { background-color: #d9534f; color: white; }");
    connect(deleteButton_, &QPushButton::clicked, this, &RegionManagerDialog::onDeleteClicked);
    buttonLayout->addWidget(deleteButton_);

    deleteAllButton_ = new QPushButton("Delete All");
    deleteAllButton_->setStyleSheet("QPushButton { background-color: #c9302c; color: white; }");
    connect(deleteAllButton_, &QPushButton::clicked, this, &RegionManagerDialog::onDeleteAllClicked);
    buttonLayout->addWidget(deleteAllButton_);

    buttonLayout->addStretch();

    closeButton_ = new QPushButton("Close");
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton_);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void RegionManagerDialog::populateList() {
    regionListWidget_->clear();

    for (size_t i = 0; i < regions_.size(); ++i) {
        const Region& region = regions_[i];
        QString itemText = QString("%1. %2 (%3 points)")
            .arg(i + 1)
            .arg(QString::fromStdString(region.getName()))
            .arg(region.getPoints().size());

        regionListWidget_->addItem(itemText);
    }

    infoLabel_->setText(QString("Total Regions: %1").arg(regions_.size()));
}

void RegionManagerDialog::updateButtonStates() {
    bool hasSelection = regionListWidget_->currentRow() >= 0;
    bool hasRegions = !regions_.empty();

    renameButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
    deleteAllButton_->setEnabled(hasRegions);
}

void RegionManagerDialog::onRenameClicked() {
    int currentRow = regionListWidget_->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(regions_.size())) {
        return;
    }

    Region& region = regions_[currentRow];
    bool ok;
    QString newName = QInputDialog::getText(
        this,
        "Rename Region",
        "Enter new name for the region:",
        QLineEdit::Normal,
        QString::fromStdString(region.getName()),
        &ok
    );

    if (ok && !newName.isEmpty()) {
        region.setName(newName.toStdString());
        populateList();
        regionListWidget_->setCurrentRow(currentRow);
    }
}

void RegionManagerDialog::onDeleteClicked() {
    int currentRow = regionListWidget_->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(regions_.size())) {
        return;
    }

    const Region& region = regions_[currentRow];
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Region",
        QString("Are you sure you want to delete region '%1'?")
            .arg(QString::fromStdString(region.getName())),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        regions_.erase(regions_.begin() + currentRow);
        populateList();
        updateButtonStates();
    }
}

void RegionManagerDialog::onDeleteAllClicked() {
    if (regions_.empty()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete All Regions",
        QString("Are you sure you want to delete ALL %1 regions?\nThis action cannot be undone.")
            .arg(regions_.size()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        regions_.clear();
        populateList();
        updateButtonStates();
    }
}
