#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QImage>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDateTime>
#include <memory>
#include <map>
#include <vector>

#include "CameraSource.h"
#include "inference.h"
#include "ByteTrack/BYTETracker.h"
#include "Region.h"
#include "RegionDrawingWidget.h"
#include "DetectionEvent.h"
#include "EventManager.h"
#include "TelegramBot.h"
#include "RegionCountManager.h"

class CameraWidget : public QWidget {
    Q_OBJECT

public:
    explicit CameraWidget(std::shared_ptr<CameraSource> camera,
                         std::shared_ptr<Inference> inference,
                         QWidget* parent = nullptr);
    ~CameraWidget();

    int getCameraId() const { return camera_->getId(); }
    bool isRunning() const { return isRunning_; }
    void updateInference(std::shared_ptr<Inference> inference);

    // Display settings
    void setDisplaySize(int width, int height);

    // Region management
    const std::vector<Region>& getRegions() const { return regions_; }
    void setRegions(const std::vector<Region>& regions) { regions_ = regions; }

public slots:
    void startCapture();
    void stopCapture();
    void toggleCapture();

signals:
    void cameraRemoved(int cameraId);
    void cropDetected(const QPixmap& cropImage, const QPixmap& fullFrameImage,
                      const QString& cameraName, const QString& className,
                      int trackId, float confidence);
    void requestFullScreen(int cameraId);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void updateFrame();
    void onRemoveClicked();
    void onDrawRegion();
    void onManageRegions();
    void onRegionCompleted(const std::vector<cv::Point>& points);

private:
    void setupUI();
    void processFrame(cv::Mat& frame);
    void drawRegionsOnFrame(cv::Mat& frame);
    cv::Mat drawDetections(cv::Mat& frame, const std::vector<byte_track::BYTETracker::STrackPtr>& tracks);
    QImage cvMatToQImage(const cv::Mat& mat);

    std::shared_ptr<CameraSource> camera_;
    std::shared_ptr<Inference> inference_;
    std::unique_ptr<byte_track::BYTETracker> tracker_;

    QString cameraName_;
    RegionDrawingWidget* videoLabel_;
    QLabel* infoLabel_;
    QPushButton* startStopButton_;
    QPushButton* removeButton_;
    QTimer* timer_;

    bool isRunning_;
    cv::Mat currentFrame_;

    // Track ID -> class ID mapping for consistent labeling
    std::map<size_t, int> trackClassMap_;

    // Region-based detection
    std::vector<Region> regions_;

    // Unique object tracking per region
    std::map<std::string, std::set<size_t>> regionUniqueObjectIds_;

    // Event capture state tracking
    struct TrackRegionState {
        bool inRegion = false;
        std::string regionName;
        int lastCaptureFrame = 0;
        int entryFrame = 0;
    };
    std::map<size_t, TrackRegionState> trackRegionStates_;
    int currentFrameNumber_;

    // Telegram throttling (region name -> last send timestamp in ms)
    std::map<std::string, qint64> lastTelegramSendTime_;
    static constexpr int TELEGRAM_THROTTLE_MS = 5000;  // 5 seconds

    void captureEvent(size_t trackId, const std::string& regionName,
                      const cv::Rect& bbox, EventType eventType);
};

#endif // CAMERAWIDGET_H
