#include "Region.h"
#include <random>

Region::Region() {
    name_ = "Unnamed Region";
    generateRandomColor();
}

Region::Region(const std::string& name, const std::vector<cv::Point>& points)
    : name_(name), points_(points) {
    generateRandomColor();
}

void Region::generateRandomColor() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(50, 255);

    color_ = cv::Scalar(dis(gen), dis(gen), dis(gen));
}

bool Region::containsPoint(const cv::Point& point) const {
    if (points_.size() < 3) {
        return false;  // Need at least 3 points for a polygon
    }

    // Use OpenCV's pointPolygonTest
    // Returns positive if inside, negative if outside, 0 if on edge
    double result = cv::pointPolygonTest(points_, cv::Point2f(point.x, point.y), false);
    return result >= 0;
}

bool Region::containsRect(const cv::Rect& rect) const {
    // Check if the center of the bounding box is inside the region
    cv::Point center(rect.x + rect.width / 2, rect.y + rect.height / 2);
    return containsPoint(center);
}

cv::Rect Region::getBoundingBox() const {
    if (points_.empty()) {
        return cv::Rect(0, 0, 0, 0);
    }

    int minX = points_[0].x, maxX = points_[0].x;
    int minY = points_[0].y, maxY = points_[0].y;

    for (const auto& pt : points_) {
        minX = std::min(minX, pt.x);
        maxX = std::max(maxX, pt.x);
        minY = std::min(minY, pt.y);
        maxY = std::max(maxY, pt.y);
    }

    return cv::Rect(minX, minY, maxX - minX, maxY - minY);
}

json Region::toJson() const {
    json j;
    j["name"] = name_;
    j["points"] = json::array();

    for (const auto& pt : points_) {
        j["points"].push_back({{"x", pt.x}, {"y", pt.y}});
    }

    j["color"] = {
        {"b", static_cast<int>(color_[0])},
        {"g", static_cast<int>(color_[1])},
        {"r", static_cast<int>(color_[2])}
    };

    return j;
}

Region Region::fromJson(const json& j) {
    Region region;

    if (j.contains("name")) {
        region.name_ = j["name"].get<std::string>();
    }

    if (j.contains("points")) {
        region.points_.clear();
        for (const auto& pt : j["points"]) {
            int x = pt["x"].get<int>();
            int y = pt["y"].get<int>();
            region.points_.push_back(cv::Point(x, y));
        }
    }

    if (j.contains("color")) {
        int b = j["color"]["b"].get<int>();
        int g = j["color"]["g"].get<int>();
        int r = j["color"]["r"].get<int>();
        region.color_ = cv::Scalar(b, g, r);
    }

    return region;
}
