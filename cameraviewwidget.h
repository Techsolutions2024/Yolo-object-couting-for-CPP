#ifndef CAMERAVIEWWIDGET_H
#define CAMERAVIEWWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>

/**
 * CameraViewWidget - Widget đại diện cho một ô camera trong grid
 * Hiển thị "Add Camera +" khi chưa có video
 */
class CameraViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CameraViewWidget(QWidget *parent = nullptr);
    ~CameraViewWidget();

    // Thiết lập trạng thái có camera hay chưa
    void setHasCamera(bool hasCamera);
    bool hasCamera() const { return m_hasCamera; }

    // Thiết lập ID camera
    void setCameraId(int id) { m_cameraId = id; }
    int cameraId() const { return m_cameraId; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void clicked();
    void addCameraRequested(int cameraId);

private:
    QLabel *m_label;
    bool m_hasCamera;
    int m_cameraId;
    bool m_isHovered;
};

#endif // CAMERAVIEWWIDGET_H
