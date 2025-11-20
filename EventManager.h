#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "DetectionEvent.h"
#include <vector>
#include <mutex>
#include <memory>
#include <QDateTime>
#include <opencv2/opencv.hpp>

class EventManager {
public:
    // Singleton access
    static EventManager& getInstance();

    // Delete copy/move constructors and assignment operators
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;
    EventManager(EventManager&&) = delete;
    EventManager& operator=(EventManager&&) = delete;

    // Event management
    void addEvent(const DetectionEvent& event);
    std::vector<DetectionEvent> getAllEvents() const;
    std::vector<DetectionEvent> getEventsByCamera(int cameraId) const;
    std::vector<DetectionEvent> getEventsByTimeRange(const QDateTime& start, const QDateTime& end) const;
    void clearEvents();
    int getEventCount() const;

    // Image management
    std::string saveEventImage(
        const cv::Mat& croppedImage,
        const std::string& cameraName,
        const std::string& regionName,
        size_t trackId,
        EventType eventType
    );

    // Configuration
    void setBaseDirectory(const std::string& dir) { baseDirectory_ = dir; }
    std::string getBaseDirectory() const { return baseDirectory_; }
    void setPeriodicCaptureInterval(int frames) { periodicCaptureInterval_ = frames; }
    int getPeriodicCaptureInterval() const { return periodicCaptureInterval_; }

    // Load events from disk
    bool loadEventsFromDirectory();

private:
    EventManager();
    ~EventManager() = default;

    std::string generateFilename(size_t trackId, EventType eventType) const;
    std::string ensureDirectoryExists(const std::string& cameraName, const std::string& regionName) const;
    void saveMetadataJson(const std::string& directory) const;

    mutable std::mutex mutex_;
    std::vector<DetectionEvent> events_;
    std::string baseDirectory_;
    int periodicCaptureInterval_;  // Frames between periodic captures
};

#endif // EVENTMANAGER_H
