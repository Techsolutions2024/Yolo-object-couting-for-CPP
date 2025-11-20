#include "FullScreenCameraView.h"
#include <QApplication>
#include <QScreen>
#include <opencv2/opencv.hpp>

FullScreenCameraView::FullScreenCameraView(std::shared_ptr<CameraSource> camera,
                                         std::shared_ptr<Inference> inference,
                                         QWidget* parent)
    : QDialog(parent), camera_(camera), inference_(inference) {

    cameraName_ = QString::fromStdString(camera_->getName());

    setupUI();

    // Open camera if not already opened
    if (!camera_->isOpened()) {
        camera_->open();
    }

    // Start frame updates
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &FullScreenCameraView::updateFrame);
    timer_->start(33); // ~30 FPS
}

FullScreenCameraView::~FullScreenCameraView() {
    timer_->stop();
}

void FullScreenCameraView::setupUI() {
    // Set window to full screen
    setWindowState(Qt::WindowFullScreen);
    setWindowTitle(QString("Full Screen - %1").arg(cameraName_));

    // Create layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Create video label
    videoLabel_ = new QLabel(this);
    videoLabel_->setAlignment(Qt::AlignCenter);
    videoLabel_->setStyleSheet("QLabel { background-color: black; }");
    videoLabel_->setScaledContents(true);

    // Set size to screen size
    QScreen* screen = QApplication::primaryScreen();
    QSize screenSize = screen->size();
    videoLabel_->setMinimumSize(screenSize);

    layout->addWidget(videoLabel_);
    setLayout(layout);

    // Show instructions overlay initially
    videoLabel_->setText(QString("<font color='white' size='5'>%1<br><br>"
                                "Press ESC or Double-Click to exit full screen</font>")
                        .arg(cameraName_));
}

void FullScreenCameraView::updateFrame() {
    if (!camera_->read(currentFrame_) || currentFrame_.empty()) {
        videoLabel_->setText("<font color='red' size='5'>Camera feed lost</font>");
        return;
    }

    // Simple detection visualization (without ByteTrack for simplicity)
    std::vector<Detection> detections = inference_->runInference(currentFrame_);

    // Draw detections
    for (const auto& det : detections) {
        cv::Scalar color(0, 255, 0);
        cv::rectangle(currentFrame_, det.box, color, 2);

        std::string label = inference_->getClassName(det.class_id) + ": " +
                          std::to_string(static_cast<int>(det.confidence * 100)) + "%";

        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = 0.6;
        int thickness = 2;

        cv::Size textSize = cv::getTextSize(label, fontFace, fontScale, thickness, 0);
        cv::Rect textBox(det.box.x, det.box.y - textSize.height - 10,
                        textSize.width + 10, textSize.height + 10);

        cv::rectangle(currentFrame_, textBox, color, cv::FILLED);
        cv::putText(currentFrame_, label, cv::Point(det.box.x + 5, det.box.y - 8),
                   fontFace, fontScale, cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);
    }

    // Draw camera name overlay
    std::string overlayText = cameraName_.toStdString() + " | Full Screen (Press ESC to exit)";
    cv::Size textSize = cv::getTextSize(overlayText, cv::FONT_HERSHEY_DUPLEX, 1.0, 2, 0);
    cv::Rect textBox(10, 10, textSize.width + 20, textSize.height + 20);
    cv::rectangle(currentFrame_, textBox, cv::Scalar(0, 0, 0), cv::FILLED);
    cv::putText(currentFrame_, overlayText, cv::Point(20, 35),
               cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);

    // Display detection count
    std::string countText = "Detections: " + std::to_string(detections.size());
    cv::Size countSize = cv::getTextSize(countText, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, 0);
    int countY = currentFrame_.rows - 20;
    cv::Rect countBox(10, countY - countSize.height - 10, countSize.width + 20, countSize.height + 20);
    cv::rectangle(currentFrame_, countBox, cv::Scalar(0, 0, 0), cv::FILLED);
    cv::putText(currentFrame_, countText, cv::Point(20, countY - 5),
               cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

    // Convert to QImage and display
    QImage qimg = cvMatToQImage(currentFrame_);
    videoLabel_->setPixmap(QPixmap::fromImage(qimg));
}

QImage FullScreenCameraView::cvMatToQImage(const cv::Mat& mat) {
    if (mat.empty()) {
        return QImage();
    }

    cv::Mat rgb;
    if (mat.channels() == 1) {
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    } else {
        rgb = mat.clone();
    }

    return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
}

void FullScreenCameraView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        accept(); // Close dialog
    } else {
        QDialog::keyPressEvent(event);
    }
}

void FullScreenCameraView::mouseDoubleClickEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    accept(); // Close dialog on double-click
}
