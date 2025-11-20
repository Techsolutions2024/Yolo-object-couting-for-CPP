#ifndef FULLSCREENCAMERAVIEW_H
#define FULLSCREENCAMERAVIEW_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QKeyEvent>
#include <memory>
#include "CameraSource.h"
#include "inference.h"

/**
 * @brief Full screen camera view dialog
 *
 * Displays a single camera in full screen mode.
 * Press ESC or double-click to exit.
 */
class FullScreenCameraView : public QDialog {
    Q_OBJECT

public:
    explicit FullScreenCameraView(std::shared_ptr<CameraSource> camera,
                                 std::shared_ptr<Inference> inference,
                                 QWidget* parent = nullptr);
    ~FullScreenCameraView();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void updateFrame();

private:
    void setupUI();
    QImage cvMatToQImage(const cv::Mat& mat);

    std::shared_ptr<CameraSource> camera_;
    std::shared_ptr<Inference> inference_;
    QLabel* videoLabel_;
    QTimer* timer_;
    cv::Mat currentFrame_;
    QString cameraName_;
};

#endif // FULLSCREENCAMERAVIEW_H
