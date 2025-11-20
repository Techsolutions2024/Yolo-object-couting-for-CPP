#ifndef REGION_H
#define REGION_H

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Region {
public:
    Region();
    Region(const std::string& name, const std::vector<cv::Point>& points);

    // Getters
    const std::string& getName() const { return name_; }
    const std::vector<cv::Point>& getPoints() const { return points_; }
    cv::Scalar getColor() const { return color_; }

    // Setters
    void setName(const std::string& name) { name_ = name; }
    void setPoints(const std::vector<cv::Point>& points) { points_ = points; }
    void setColor(const cv::Scalar& color) { color_ = color; }

    // Geometry utilities
    bool containsPoint(const cv::Point& point) const;
    bool containsRect(const cv::Rect& rect) const;  // Check if rect center is inside
    cv::Rect getBoundingBox() const;

    // Serialization
    json toJson() const;
    static Region fromJson(const json& j);

private:
    std::string name_;
    std::vector<cv::Point> points_;
    cv::Scalar color_;  // For visualization

    void generateRandomColor();
};

#endif // REGION_H
