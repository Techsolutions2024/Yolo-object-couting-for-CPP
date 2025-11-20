#include "CameraSource.h"
#include <iostream>
#include <thread>
#include <chrono>

using json = nlohmann::json;

CameraSource::CameraSource(int id, const std::string& name, CameraType type, const std::string& source)
    : id_(id), name_(name), type_(type), source_(source), isActive_(false) {
}

CameraSource::~CameraSource() {
    close();
}

void CameraSource::setSource(const std::string& source) {
    bool wasOpened = isOpened();
    if (wasOpened) {
        close();
    }
    source_ = source;
    if (wasOpened) {
        open();
    }
}

bool CameraSource::open() {
    if (isOpened()) {
        return true;
    }

    try {
        if (type_ == CameraType::WEBCAM) {
            int deviceId = std::stoi(source_);
            capture_.open(deviceId);
        } else {
            capture_.open(source_);
        }

        if (capture_.isOpened()) {
            isActive_ = true;
            std::cout << "Camera '" << name_ << "' opened successfully." << std::endl;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error opening camera '" << name_ << "': " << e.what() << std::endl;
    }

    isActive_ = false;
    return false;
}

void CameraSource::close() {
    if (capture_.isOpened()) {
        capture_.release();
        isActive_ = false;
        std::cout << "Camera '" << name_ << "' closed." << std::endl;
    }
}

bool CameraSource::read(cv::Mat& frame) {
    if (!isOpened()) {
        return false;
    }

    bool success = capture_.read(frame);
    if (!success) {
        std::cerr << "Failed to read frame from camera '" << name_ << "'" << std::endl;
        isActive_ = false;
    }
    return success;
}

void CameraSource::reconnect() {
    std::cout << "Attempting to reconnect camera '" << name_ << "'..." << std::endl;
    close();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    open();
}

int CameraSource::getOpenCVCaptureParam() const {
    switch (type_) {
        case CameraType::WEBCAM:
            return cv::CAP_ANY;
        case CameraType::VIDEO_FILE:
            return cv::CAP_FFMPEG;
        case CameraType::RTSP_STREAM:
        case CameraType::IP_CAMERA:
            return cv::CAP_FFMPEG;
        default:
            return cv::CAP_ANY;
    }
}

json CameraSource::toJson() const {
    return json{
        {"id", id_},
        {"name", name_},
        {"type", cameraTypeToString(type_)},
        {"source", source_}
    };
}

CameraSource CameraSource::fromJson(const json& j) {
    return CameraSource(
        j.at("id").get<int>(),
        j.at("name").get<std::string>(),
        stringToCameraType(j.at("type").get<std::string>()),
        j.at("source").get<std::string>()
    );
}

std::string CameraSource::cameraTypeToString(CameraType type) {
    switch (type) {
        case CameraType::WEBCAM: return "webcam";
        case CameraType::VIDEO_FILE: return "video_file";
        case CameraType::RTSP_STREAM: return "rtsp_stream";
        case CameraType::IP_CAMERA: return "ip_camera";
        default: return "unknown";
    }
}

CameraType CameraSource::stringToCameraType(const std::string& typeStr) {
    if (typeStr == "webcam") return CameraType::WEBCAM;
    if (typeStr == "video_file") return CameraType::VIDEO_FILE;
    if (typeStr == "rtsp_stream") return CameraType::RTSP_STREAM;
    if (typeStr == "ip_camera") return CameraType::IP_CAMERA;
    return CameraType::WEBCAM; // default
}
