#include "EventManager.h"
#include <QDir>
#include <QDirIterator>
#include <QImage>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

EventManager::EventManager()
    : baseDirectory_("events"), periodicCaptureInterval_(30) {
}

EventManager& EventManager::getInstance() {
    static EventManager instance;
    return instance;
}

void EventManager::addEvent(const DetectionEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(event);
}

std::vector<DetectionEvent> EventManager::getAllEvents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
}

std::vector<DetectionEvent> EventManager::getEventsByCamera(int cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DetectionEvent> filtered;

    for (const auto& event : events_) {
        if (event.getCameraId() == cameraId) {
            filtered.push_back(event);
        }
    }

    return filtered;
}

std::vector<DetectionEvent> EventManager::getEventsByTimeRange(
    const QDateTime& start,
    const QDateTime& end
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DetectionEvent> filtered;

    for (const auto& event : events_) {
        QDateTime eventTime = event.getTimestamp();
        if (eventTime >= start && eventTime <= end) {
            filtered.push_back(event);
        }
    }

    return filtered;
}

void EventManager::clearEvents() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
}

int EventManager::getEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(events_.size());
}

std::string EventManager::ensureDirectoryExists(
    const std::string& cameraName,
    const std::string& regionName
) const {
    // Get current date for directory structure
    QDateTime now = QDateTime::currentDateTime();
    QString dateStr = now.toString("yyyy-MM-dd");

    // Build path: events/CameraName/RegionName/YYYY-MM-DD/
    QString basePath = QString::fromStdString(baseDirectory_);
    QString cameraPath = QString::fromStdString(cameraName);
    QString regionPath = QString::fromStdString(regionName);

    // Replace spaces with underscores for valid directory names
    cameraPath.replace(" ", "_");
    regionPath.replace(" ", "_");

    QString fullPath = basePath + "/" + cameraPath + "/" + regionPath + "/" + dateStr;

    // Create directories if they don't exist
    QDir dir;
    if (!dir.exists(fullPath)) {
        dir.mkpath(fullPath);
    }

    return fullPath.toStdString();
}

std::string EventManager::generateFilename(size_t trackId, EventType eventType) const {
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("HHmmss");

    std::ostringstream oss;
    oss << trackId << "_" << timeStr.toStdString() << "_";

    switch (eventType) {
        case EventType::FIRST_ENTRY:
            oss << "ENTRY";
            break;
        case EventType::PERIODIC:
            oss << "PERIODIC";
            break;
        case EventType::EXIT:
            oss << "EXIT";
            break;
    }

    oss << ".jpg";
    return oss.str();
}

std::string EventManager::saveEventImage(
    const cv::Mat& croppedImage,
    const std::string& cameraName,
    const std::string& regionName,
    size_t trackId,
    EventType eventType
) {
    if (croppedImage.empty()) {
        return "";
    }

    try {
        // Ensure directory exists
        std::string directory = ensureDirectoryExists(cameraName, regionName);

        // Generate filename
        std::string filename = generateFilename(trackId, eventType);
        std::string fullPath = directory + "/" + filename;

        // Save image
        cv::imwrite(fullPath, croppedImage);

        return fullPath;
    } catch (const std::exception& e) {
        std::cerr << "Error saving event image: " << e.what() << std::endl;
        return "";
    }
}

void EventManager::saveMetadataJson(const std::string& directory) const {
    try {
        json j;
        j["events"] = json::array();

        // Filter events for this directory
        for (const auto& event : events_) {
            std::string eventPath = event.getImagePath();
            if (eventPath.find(directory) != std::string::npos) {
                j["events"].push_back(event.toJson());
            }
        }

        std::string metadataPath = directory + "/metadata.json";
        std::ofstream file(metadataPath);
        file << j.dump(4);
        file.close();
    } catch (const std::exception& e) {
        std::cerr << "Error saving metadata JSON: " << e.what() << std::endl;
    }
}

bool EventManager::loadEventsFromDirectory() {
    std::lock_guard<std::mutex> lock(mutex_);

    QDir baseDir(QString::fromStdString(baseDirectory_));
    if (!baseDir.exists()) {
        std::cout << "⚠️  Events directory not found: " << baseDirectory_ << std::endl;
        return false;
    }

    int loadedCount = 0;
    std::cout << "📂 Loading events from: " << baseDirectory_ << std::endl;

    // Recursively find all metadata.json files
    QDirIterator it(QString::fromStdString(baseDirectory_),
                    QStringList() << "metadata.json",
                    QDir::Files,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString metadataPath = it.next();

        try {
            // Read JSON file
            QFile file(metadataPath);
            if (!file.open(QIODevice::ReadOnly)) {
                std::cerr << "❌ Cannot open: " << metadataPath.toStdString() << std::endl;
                continue;
            }

            QByteArray data = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isNull() || !doc.isObject()) {
                std::cerr << "❌ Invalid JSON: " << metadataPath.toStdString() << std::endl;
                continue;
            }

            QJsonObject rootObj = doc.object();
            QJsonArray eventsArray = rootObj["events"].toArray();

            // Load each event from the metadata
            for (const QJsonValue& eventValue : eventsArray) {
                if (!eventValue.isObject()) continue;

                QJsonObject eventObj = eventValue.toObject();

                // Convert Qt JSON to nlohmann json
                std::string jsonStr = QString(QJsonDocument(eventObj).toJson()).toStdString();
                json j = json::parse(jsonStr);

                // Create DetectionEvent from JSON
                DetectionEvent event = DetectionEvent::fromJson(j);
                events_.push_back(event);
                loadedCount++;
            }

            std::cout << "  ✅ Loaded " << eventsArray.size()
                     << " events from: " << metadataPath.toStdString() << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "❌ Error loading " << metadataPath.toStdString()
                     << ": " << e.what() << std::endl;
        }
    }

    std::cout << "✅ Total events loaded: " << loadedCount << std::endl;
    return loadedCount > 0;
}
