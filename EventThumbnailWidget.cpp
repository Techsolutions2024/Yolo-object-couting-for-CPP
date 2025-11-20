#include "EventThumbnailWidget.h"
#include <QFile>
#include <opencv2/opencv.hpp>

EventThumbnailWidget::EventThumbnailWidget(const DetectionEvent& event, QWidget* parent)
    : QWidget(parent), event_(event) {
    setupUI();
}

void EventThumbnailWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2);

    // Image label
    imageLabel_ = new QLabel();
    imageLabel_->setFixedSize(150, 150);
    imageLabel_->setScaledContents(true);
    imageLabel_->setStyleSheet("QLabel { background-color: #2c2c2c; border: 2px solid #444; }");
    imageLabel_->setAlignment(Qt::AlignCenter);

    // Load and display thumbnail
    QPixmap thumbnail = loadThumbnail();
    if (!thumbnail.isNull()) {
        imageLabel_->setPixmap(thumbnail);
    } else {
        imageLabel_->setText("No Image");
        imageLabel_->setStyleSheet("QLabel { color: #888; background-color: #2c2c2c; border: 2px solid #444; }");
    }

    layout->addWidget(imageLabel_);

    // Info label
    infoLabel_ = new QLabel();
    QString infoText = QString("<b>%1</b><br>%2<br>%3<br><small>%4</small>")
        .arg(QString::fromStdString(event_.getCameraName()))
        .arg(QString::fromStdString(event_.getRegionName()))
        .arg(QString::fromStdString(event_.getObjectClass()))
        .arg(event_.getTimestamp().toString("MM/dd HH:mm:ss"));

    infoLabel_->setText(infoText);
    infoLabel_->setAlignment(Qt::AlignCenter);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("QLabel { font-size: 9pt; padding: 2px; }");

    layout->addWidget(infoLabel_);

    setLayout(layout);
    setFixedWidth(160);

    // Set cursor to pointer to indicate clickability
    setCursor(Qt::PointingHandCursor);
}

QPixmap EventThumbnailWidget::loadThumbnail() {
    std::string imagePath = event_.getImagePath();

    if (imagePath.empty()) {
        return QPixmap();
    }

    // Try loading with OpenCV first
    cv::Mat img = cv::imread(imagePath);
    if (img.empty()) {
        return QPixmap();
    }

    // Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(img, rgb, cv::COLOR_BGR2RGB);

    // Convert to QImage
    QImage qimg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    return QPixmap::fromImage(qimg.copy());
}

void EventThumbnailWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(event_);
    }
    QWidget::mousePressEvent(event);
}

void EventThumbnailWidget::enterEvent(QEnterEvent* event) {
    // Highlight on hover
    imageLabel_->setStyleSheet("QLabel { background-color: #3c3c3c; border: 2px solid #5599ff; }");

    // Show detailed tooltip
    QString tooltip = QString(
        "<b>Track ID:</b> %1<br>"
        "<b>Camera:</b> %2<br>"
        "<b>Region:</b> %3<br>"
        "<b>Class:</b> %4<br>"
        "<b>Confidence:</b> %5%<br>"
        "<b>Event Type:</b> %6<br>"
        "<b>Time:</b> %7"
    )
        .arg(event_.getTrackId())
        .arg(QString::fromStdString(event_.getCameraName()))
        .arg(QString::fromStdString(event_.getRegionName()))
        .arg(QString::fromStdString(event_.getObjectClass()))
        .arg(static_cast<int>(event_.getConfidence() * 100))
        .arg(QString::fromStdString(event_.getEventTypeString()))
        .arg(event_.getTimestamp().toString("yyyy-MM-dd HH:mm:ss"));

    setToolTip(tooltip);

    QWidget::enterEvent(event);
}

void EventThumbnailWidget::leaveEvent(QEvent* event) {
    // Remove highlight
    imageLabel_->setStyleSheet("QLabel { background-color: #2c2c2c; border: 2px solid #444; }");
    QWidget::leaveEvent(event);
}
