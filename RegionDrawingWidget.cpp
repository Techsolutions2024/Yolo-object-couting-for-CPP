#include "RegionDrawingWidget.h"
#include <QMessageBox>

RegionDrawingWidget::RegionDrawingWidget(QWidget* parent)
    : QLabel(parent), drawingEnabled_(false), tracking_(false) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void RegionDrawingWidget::setEnabled(bool enabled) {
    drawingEnabled_ = enabled;
    tracking_ = enabled;
    if (!enabled) {
        points_.clear();
        update();
    }
}

void RegionDrawingWidget::clearPoints() {
    points_.clear();
    update();
}

void RegionDrawingWidget::setImageSize(const QSize& size) {
    imageSize_ = size;
}

QPoint RegionDrawingWidget::scaledToOriginal(const QPoint& point) {
    if (imageSize_.isEmpty() || size().isEmpty()) {
        return point;
    }

    // Calculate scaling factors
    double scaleX = static_cast<double>(imageSize_.width()) / size().width();
    double scaleY = static_cast<double>(imageSize_.height()) / size().height();

    return QPoint(
        static_cast<int>(point.x() * scaleX),
        static_cast<int>(point.y() * scaleY)
    );
}

void RegionDrawingWidget::mousePressEvent(QMouseEvent* event) {
    if (!drawingEnabled_) {
        QLabel::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Add point
        QPoint scaledPoint = scaledToOriginal(event->pos());
        points_.push_back(cv::Point(scaledPoint.x(), scaledPoint.y()));
        update();
    } else if (event->button() == Qt::RightButton) {
        // Complete the region
        if (points_.size() >= 3) {
            emit regionCompleted(points_);
            points_.clear();
            drawingEnabled_ = false;
            tracking_ = false;
        } else {
            QMessageBox::warning(this, "Invalid Region",
                "Please add at least 3 points to create a region.\n"
                "Left-click to add points, right-click to finish.");
        }
        update();
    }

    QLabel::mousePressEvent(event);
}

void RegionDrawingWidget::mouseMoveEvent(QMouseEvent* event) {
    if (drawingEnabled_ && tracking_) {
        currentMousePos_ = event->pos();
        update();
    }
    QLabel::mouseMoveEvent(event);
}

void RegionDrawingWidget::paintEvent(QPaintEvent* event) {
    QLabel::paintEvent(event);

    if (!drawingEnabled_ || points_.empty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Calculate reverse scaling (original to display)
    double scaleX = 1.0;
    double scaleY = 1.0;
    if (!imageSize_.isEmpty() && !size().isEmpty()) {
        scaleX = static_cast<double>(size().width()) / imageSize_.width();
        scaleY = static_cast<double>(size().height()) / imageSize_.height();
    }

    // Draw lines between points
    painter.setPen(QPen(Qt::yellow, 2));
    for (size_t i = 0; i < points_.size(); ++i) {
        QPoint p1(static_cast<int>(points_[i].x * scaleX),
                  static_cast<int>(points_[i].y * scaleY));

        if (i < points_.size() - 1) {
            QPoint p2(static_cast<int>(points_[i + 1].x * scaleX),
                      static_cast<int>(points_[i + 1].y * scaleY));
            painter.drawLine(p1, p2);
        } else {
            // Draw line from last point to mouse cursor
            painter.setPen(QPen(Qt::yellow, 1, Qt::DashLine));
            painter.drawLine(p1, currentMousePos_);
        }

        // Draw points as circles
        painter.setPen(QPen(Qt::red, 1));
        painter.setBrush(Qt::red);
        painter.drawEllipse(p1, 4, 4);
    }

    // Draw closing line preview if we have at least 2 points
    if (points_.size() >= 2) {
        painter.setPen(QPen(Qt::green, 1, Qt::DashLine));
        QPoint first(static_cast<int>(points_[0].x * scaleX),
                     static_cast<int>(points_[0].y * scaleY));
        painter.drawLine(currentMousePos_, first);
    }

    // Draw instruction text
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(10, 20, QString("Left-click: Add point | Right-click: Finish region | Points: %1")
                     .arg(points_.size()));
}
