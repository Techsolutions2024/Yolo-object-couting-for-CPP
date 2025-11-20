#include "DetectionEvent.h"

DetectionEvent::DetectionEvent()
    : trackId_(0), cameraId_(0), confidence_(0.0f),
      eventType_(EventType::FIRST_ENTRY), frameNumber_(0) {
    timestamp_ = QDateTime::currentDateTime();
}

DetectionEvent::DetectionEvent(
    size_t trackId,
    int cameraId,
    const std::string& cameraName,
    const std::string& regionName,
    const std::string& objectClass,
    float confidence,
    EventType eventType,
    const cv::Rect& bbox,
    const std::string& imagePath
) : trackId_(trackId), cameraId_(cameraId), cameraName_(cameraName),
    regionName_(regionName), objectClass_(objectClass), confidence_(confidence),
    eventType_(eventType), bbox_(bbox), imagePath_(imagePath), frameNumber_(0) {
    timestamp_ = QDateTime::currentDateTime();
}

std::string DetectionEvent::getEventTypeString() const {
    switch (eventType_) {
        case EventType::FIRST_ENTRY: return "ENTRY";
        case EventType::PERIODIC: return "PERIODIC";
        case EventType::EXIT: return "EXIT";
        default: return "UNKNOWN";
    }
}

EventType DetectionEvent::stringToEventType(const std::string& typeStr) {
    if (typeStr == "ENTRY" || typeStr == "FIRST_ENTRY") return EventType::FIRST_ENTRY;
    if (typeStr == "PERIODIC") return EventType::PERIODIC;
    if (typeStr == "EXIT") return EventType::EXIT;
    return EventType::FIRST_ENTRY;
}

json DetectionEvent::toJson() const {
    json j;
    j["track_id"] = trackId_;
    j["camera_id"] = cameraId_;
    j["camera_name"] = cameraName_;
    j["region_name"] = regionName_;
    j["object_class"] = objectClass_;
    j["confidence"] = confidence_;
    j["event_type"] = getEventTypeString();
    j["timestamp"] = timestamp_.toString("yyyy-MM-dd HH:mm:ss").toStdString();
    j["frame_number"] = frameNumber_;
    j["image_path"] = imagePath_;
    j["bbox"] = {
        {"x", bbox_.x},
        {"y", bbox_.y},
        {"width", bbox_.width},
        {"height", bbox_.height}
    };
    return j;
}

DetectionEvent DetectionEvent::fromJson(const json& j) {
    DetectionEvent event;

    if (j.contains("track_id")) event.trackId_ = j["track_id"].get<size_t>();
    if (j.contains("camera_id")) event.cameraId_ = j["camera_id"].get<int>();
    if (j.contains("camera_name")) event.cameraName_ = j["camera_name"].get<std::string>();
    if (j.contains("region_name")) event.regionName_ = j["region_name"].get<std::string>();
    if (j.contains("object_class")) event.objectClass_ = j["object_class"].get<std::string>();
    if (j.contains("confidence")) event.confidence_ = j["confidence"].get<float>();
    if (j.contains("event_type")) {
        event.eventType_ = stringToEventType(j["event_type"].get<std::string>());
    }
    if (j.contains("timestamp")) {
        event.timestamp_ = QDateTime::fromString(
            QString::fromStdString(j["timestamp"].get<std::string>()),
            "yyyy-MM-dd HH:mm:ss"
        );
    }
    if (j.contains("frame_number")) event.frameNumber_ = j["frame_number"].get<int>();
    if (j.contains("image_path")) event.imagePath_ = j["image_path"].get<std::string>();
    if (j.contains("bbox")) {
        int x = j["bbox"]["x"].get<int>();
        int y = j["bbox"]["y"].get<int>();
        int width = j["bbox"]["width"].get<int>();
        int height = j["bbox"]["height"].get<int>();
        event.bbox_ = cv::Rect(x, y, width, height);
    }

    return event;
}
