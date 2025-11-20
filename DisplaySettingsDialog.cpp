#include "DisplaySettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

DisplaySettingsDialog::DisplaySettingsDialog(QWidget* parent)
    : QDialog(parent) {
    settings_ = new QSettings("YOLOTracking", "Yolov8CameraGUI", this);
    setupUI();
    loadSettings();
}

void DisplaySettingsDialog::setupUI() {
    setWindowTitle("Display Settings");
    setMinimumWidth(400);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Grid Settings Section
    QGroupBox* gridGroup = new QGroupBox("Grid Settings");
    QFormLayout* formLayout = new QFormLayout();

    widthSpinBox_ = new QSpinBox();
    widthSpinBox_->setRange(320, 1920);
    widthSpinBox_->setSingleStep(10);
    widthSpinBox_->setSuffix(" pixels");
    formLayout->addRow("Cell Width:", widthSpinBox_);

    heightSpinBox_ = new QSpinBox();
    heightSpinBox_->setRange(240, 1080);
    heightSpinBox_->setSingleStep(10);
    heightSpinBox_->setSuffix(" pixels");
    formLayout->addRow("Cell Height:", heightSpinBox_);

    rowsSpinBox_ = new QSpinBox();
    rowsSpinBox_->setRange(1, 8);
    rowsSpinBox_->setSuffix(" rows");
    formLayout->addRow("Grid Rows:", rowsSpinBox_);

    columnsSpinBox_ = new QSpinBox();
    columnsSpinBox_->setRange(1, 8);
    columnsSpinBox_->setSuffix(" columns");
    formLayout->addRow("Grid Columns:", columnsSpinBox_);

    gridGroup->setLayout(formLayout);
    mainLayout->addWidget(gridGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    resetButton_ = new QPushButton("Reset to Default");
    connect(resetButton_, &QPushButton::clicked, this, &DisplaySettingsDialog::onResetToDefault);
    buttonLayout->addWidget(resetButton_);

    buttonLayout->addStretch();

    cancelButton_ = new QPushButton("Cancel");
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton_);

    saveButton_ = new QPushButton("Save");
    saveButton_->setStyleSheet("QPushButton { background-color: #5cb85c; color: white; font-weight: bold; }");
    connect(saveButton_, &QPushButton::clicked, this, &DisplaySettingsDialog::onSave);
    buttonLayout->addWidget(saveButton_);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void DisplaySettingsDialog::loadSettings() {
    savedWidth_ = settings_->value("Display/CameraWidth", 640).toInt();
    savedHeight_ = settings_->value("Display/CameraHeight", 480).toInt();
    savedRows_ = settings_->value("Display/GridRows", 2).toInt();
    savedColumns_ = settings_->value("Display/GridColumns", 2).toInt();

    widthSpinBox_->setValue(savedWidth_);
    heightSpinBox_->setValue(savedHeight_);
    rowsSpinBox_->setValue(savedRows_);
    columnsSpinBox_->setValue(savedColumns_);
}

void DisplaySettingsDialog::onResetToDefault() {
    widthSpinBox_->setValue(640);
    heightSpinBox_->setValue(480);
    rowsSpinBox_->setValue(2);
    columnsSpinBox_->setValue(2);
}

void DisplaySettingsDialog::onSave() {
    savedWidth_ = widthSpinBox_->value();
    savedHeight_ = heightSpinBox_->value();
    savedRows_ = rowsSpinBox_->value();
    savedColumns_ = columnsSpinBox_->value();

    settings_->setValue("Display/CameraWidth", savedWidth_);
    settings_->setValue("Display/CameraHeight", savedHeight_);
    settings_->setValue("Display/GridRows", savedRows_);
    settings_->setValue("Display/GridColumns", savedColumns_);

    accept();
}

int DisplaySettingsDialog::getCameraWidth() const {
    return savedWidth_;
}

int DisplaySettingsDialog::getCameraHeight() const {
    return savedHeight_;
}

int DisplaySettingsDialog::getGridRows() const {
    return savedRows_;
}

int DisplaySettingsDialog::getGridColumns() const {
    return savedColumns_;
}
