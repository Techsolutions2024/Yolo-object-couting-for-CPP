#include "AddCameraDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDialogButtonBox>

AddCameraDialog::AddCameraDialog(QWidget* parent)
    : QDialog(parent) {

    setWindowTitle("Add New Camera");
    setModal(true);
    setupUI();
    resize(500, 200);
}

void AddCameraDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form layout
    QFormLayout* formLayout = new QFormLayout();

    // Camera name
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("e.g., Front Door Camera");
    formLayout->addRow("Camera Name:", nameEdit_);

    // Camera type
    typeComboBox_ = new QComboBox();
    typeComboBox_->addItem("Webcam", static_cast<int>(CameraType::WEBCAM));
    typeComboBox_->addItem("Video File", static_cast<int>(CameraType::VIDEO_FILE));
    typeComboBox_->addItem("RTSP Stream", static_cast<int>(CameraType::RTSP_STREAM));
    typeComboBox_->addItem("IP Camera", static_cast<int>(CameraType::IP_CAMERA));
    connect(typeComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddCameraDialog::onTypeChanged);
    formLayout->addRow("Camera Type:", typeComboBox_);

    // Source input with browse button
    QHBoxLayout* sourceLayout = new QHBoxLayout();
    sourceEdit_ = new QLineEdit();
    sourceEdit_->setPlaceholderText("0");
    sourceLayout->addWidget(sourceEdit_);

    browseButton_ = new QPushButton("Browse...");
    browseButton_->setVisible(false);
    connect(browseButton_, &QPushButton::clicked, this, &AddCameraDialog::onBrowseClicked);
    sourceLayout->addWidget(browseButton_);

    formLayout->addRow("Source:", sourceLayout);

    mainLayout->addLayout(formLayout);

    // Help text
    QLabel* helpLabel = new QLabel();
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; padding: 10px; }");
    helpLabel->setText(
        "<b>Examples:</b><br>"
        "• Webcam: 0 (default), 1, 2...<br>"
        "• Video File: path/to/video.mp4<br>"
        "• RTSP: rtsp://username:password@192.168.1.100:554/stream<br>"
        "• IP Camera: http://192.168.1.100:8080/video"
    );
    mainLayout->addWidget(helpLabel);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddCameraDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

void AddCameraDialog::onTypeChanged(int index) {
    CameraType type = static_cast<CameraType>(typeComboBox_->currentData().toInt());

    switch (type) {
        case CameraType::WEBCAM:
            sourceEdit_->setPlaceholderText("0");
            browseButton_->setVisible(false);
            break;

        case CameraType::VIDEO_FILE:
            sourceEdit_->setPlaceholderText("path/to/video.mp4");
            browseButton_->setVisible(true);
            break;

        case CameraType::RTSP_STREAM:
            sourceEdit_->setPlaceholderText("rtsp://username:password@192.168.1.100:554/stream");
            browseButton_->setVisible(false);
            break;

        case CameraType::IP_CAMERA:
            sourceEdit_->setPlaceholderText("http://192.168.1.100:8080/video");
            browseButton_->setVisible(false);
            break;
    }

    sourceEdit_->clear();
}

void AddCameraDialog::onBrowseClicked() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Select Video File",
        "",
        "Video Files (*.mp4 *.avi *.mkv *.mov);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        sourceEdit_->setText(filename);
    }
}

void AddCameraDialog::onOkClicked() {
    if (validateInput()) {
        accept();
    }
}

bool AddCameraDialog::validateInput() {
    if (nameEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a camera name.");
        nameEdit_->setFocus();
        return false;
    }

    if (sourceEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a camera source.");
        sourceEdit_->setFocus();
        return false;
    }

    return true;
}

QString AddCameraDialog::getCameraName() const {
    return nameEdit_->text().trimmed();
}

CameraType AddCameraDialog::getCameraType() const {
    return static_cast<CameraType>(typeComboBox_->currentData().toInt());
}

QString AddCameraDialog::getCameraSource() const {
    return sourceEdit_->text().trimmed();
}
