#ifndef DETECTIONEVENT_H
#define DETECTIONEVENT_H

#include <string>
#include <opencv2/opencv.hpp>
#include <QDateTime>
#include <QPixmap>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class EventType {
    FIRST_ENTRY,    // Object first detected in region
    PERIODIC,       // Periodic snapshot while in region
    EXIT            // Object leaving region
};

class DetectionEvent {
public:
    DetectionEvent();
    DetectionEvent(
        size_t trackId,
        int cameraId,
        const std::string& cameraName,
        const std::string& regionName,
        const std::string& objectClass,
        float confidence,
        EventType eventType,
        const cv::Rect& bbox,
        const std::string& imagePath = ""
    );

    // Getters
    size_t getTrackId() const { return trackId_; }
    int getCameraId() const { return cameraId_; }
    std::string getCameraName() const { return cameraName_; }
    std::string getRegionName() const { return regionName_; }
    std::string getObjectClass() const { return objectClass_; }
    float getConfidence() const { return confidence_; }
    EventType getEventType() const { return eventType_; }
    QDateTime getTimestamp() const { return timestamp_; }
    cv::Rect getBoundingBox() const { return bbox_; }
    std::string getImagePath() const { return imagePath_; }
    QPixmap getThumbnail() const { return thumbnail_; }
    int getFrameNumber() const { return frameNumber_; }

    // Setters
    void setImagePath(const std::string& path) { imagePath_ = path; }
    void setThumbnail(const QPixmap& pixmap) { thumbnail_ = pixmap; }
    void setFrameNumber(int frame) { frameNumber_ = frame; }

    // Utility
    std::string getEventTypeString() const;
    static EventType stringToEventType(const std::string& typeStr);

    // Serialization
    json toJson() const;
    static DetectionEvent fromJson(const json& j);

private:
    size_t trackId_;
    int cameraId_;
    std::string cameraName_;
    std::string regionName_;
    std::string objectClass_;
    float confidence_;
    EventType eventType_;
    QDateTime timestamp_;
    cv::Rect bbox_;
    std::string imagePath_;
    QPixmap thumbnail_;
    int frameNumber_;
};

#endif // DETECTIONEVENT_H
