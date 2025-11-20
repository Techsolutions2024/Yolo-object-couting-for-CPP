#ifndef EVENTTHUMBNAILWIDGET_H
#define EVENTTHUMBNAILWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QMouseEvent>
#include "DetectionEvent.h"

class EventThumbnailWidget : public QWidget {
    Q_OBJECT

public:
    explicit EventThumbnailWidget(const DetectionEvent& event, QWidget* parent = nullptr);

    const DetectionEvent& getEvent() const { return event_; }

signals:
    void clicked(const DetectionEvent& event);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupUI();
    QPixmap loadThumbnail();

    DetectionEvent event_;
    QLabel* imageLabel_;
    QLabel* infoLabel_;
};

#endif // EVENTTHUMBNAILWIDGET_H
