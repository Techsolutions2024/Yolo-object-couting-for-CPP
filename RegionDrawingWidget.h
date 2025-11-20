#ifndef REGIONDRAWINGWIDGET_H
#define REGIONDRAWINGWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <vector>
#include <opencv2/opencv.hpp>

class RegionDrawingWidget : public QLabel {
    Q_OBJECT

public:
    explicit RegionDrawingWidget(QWidget* parent = nullptr);

    void setEnabled(bool enabled);
    bool isDrawing() const { return drawingEnabled_; }
    const std::vector<cv::Point>& getPoints() const { return points_; }
    void clearPoints();
    void setImageSize(const QSize& size);

signals:
    void regionCompleted(const std::vector<cv::Point>& points);
    void drawingCancelled();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    bool drawingEnabled_;
    std::vector<cv::Point> points_;
    QPoint currentMousePos_;
    bool tracking_;
    QSize imageSize_;

    QPoint scaledToOriginal(const QPoint& point);
};

#endif // REGIONDRAWINGWIDGET_H
