#ifndef CAMERASOURCE_H
#define CAMERASOURCE_H

#include <string>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>

enum class CameraType {
    WEBCAM,
    VIDEO_FILE,
    RTSP_STREAM,
    IP_CAMERA
};

class CameraSource {
public:
    CameraSource(int id, const std::string& name, CameraType type, const std::string& source);
    ~CameraSource();

    // Getters
    int getId() const { return id_; }
    std::string getName() const { return name_; }
    CameraType getType() const { return type_; }
    std::string getSource() const { return source_; }
    bool isActive() const { return isActive_; }
    bool isOpened() const { return capture_.isOpened(); }

    // Setters
    void setName(const std::string& name) { name_ = name; }
    void setSource(const std::string& source);

    // Camera operations
    bool open();
    void close();
    bool read(cv::Mat& frame);
    void reconnect();

    // JSON serialization
    nlohmann::json toJson() const;
    static CameraSource fromJson(const nlohmann::json& j);

    // Helper function to convert CameraType to string
    static std::string cameraTypeToString(CameraType type);
    static CameraType stringToCameraType(const std::string& typeStr);

private:
    int id_;
    std::string name_;
    CameraType type_;
    std::string source_;
    bool isActive_;
    cv::VideoCapture capture_;

    // Helper to determine OpenCV capture parameter
    int getOpenCVCaptureParam() const;
};

#endif // CAMERASOURCE_H
