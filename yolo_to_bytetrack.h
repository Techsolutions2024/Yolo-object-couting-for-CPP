#pragma once

#include "inference.h"
#include "ByteTrack/BYTETracker.h"
#include <opencv2/opencv.hpp>
#include <vector>

// Convert YOLO Detection to ByteTrack Object
inline std::vector<byte_track::Object> convertToByteTrackObjects(const std::vector<Detection>& detections)
{
    std::vector<byte_track::Object> objects;
    objects.reserve(detections.size());

    for (const auto& det : detections)
    {
        // Convert cv::Rect to ByteTrack Rect<float> (x, y, width, height)
        byte_track::Rect<float> rect(
            static_cast<float>(det.box.x),
            static_cast<float>(det.box.y),
            static_cast<float>(det.box.width),
            static_cast<float>(det.box.height)
        );

        // Create ByteTrack Object
        byte_track::Object obj(rect, det.class_id, det.confidence);
        objects.push_back(obj);
    }

    return objects;
}

// Generate consistent color from track ID
inline cv::Scalar getColorForTrackID(size_t track_id)
{
    // Use track_id to generate consistent RGB color
    int r = (track_id * 123) % 256;
    int g = (track_id * 456) % 256;
    int b = (track_id * 789) % 256;

    return cv::Scalar(b, g, r); // BGR format for OpenCV
}

// Calculate IoU between two OpenCV Rects
inline float calcIoU(const cv::Rect& rect1, const cv::Rect& rect2)
{
    int x1 = std::max(rect1.x, rect2.x);
    int y1 = std::max(rect1.y, rect2.y);
    int x2 = std::min(rect1.x + rect1.width, rect2.x + rect2.width);
    int y2 = std::min(rect1.y + rect1.height, rect2.y + rect2.height);

    int intersection_width = std::max(0, x2 - x1);
    int intersection_height = std::max(0, y2 - y1);
    int intersection_area = intersection_width * intersection_height;

    int area1 = rect1.width * rect1.height;
    int area2 = rect2.width * rect2.height;
    int union_area = area1 + area2 - intersection_area;

    if (union_area <= 0)
        return 0.0f;

    return static_cast<float>(intersection_area) / static_cast<float>(union_area);
}
