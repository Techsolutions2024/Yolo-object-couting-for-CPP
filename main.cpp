// Ultralytics 🚀 AGPL-3.0 License - https://ultralytics.com/license

#include <iostream>
#include <vector>
#include <map>

#include <opencv2/opencv.hpp>

#include "inference.h"
#include "yolo_to_bytetrack.h"
#include "ByteTrack/BYTETracker.h"

using namespace std;
using namespace cv;

int main(int argc, char **argv)
{
    bool runOnGPU = false; // Set to false for CPU-only mode (no CUDA)

    // Initialize YOLOv8 model with yolov8n.onnx
    Inference inf("yolov8n.onnx", cv::Size(640, 640), "classes.txt", runOnGPU);

    // Initialize ByteTrack tracker
    // Parameters: frame_rate, track_buffer, track_thresh, high_thresh, match_thresh
    byte_track::BYTETracker tracker(30, 30, 0.5, 0.6, 0.8);

    // Open webcam (default camera = 0)
    cv::VideoCapture cap(0);

    if (!cap.isOpened())
    {
        std::cerr << "Error: Cannot open webcam!" << std::endl;
        return -1;
    }

    std::cout << "Starting webcam inference with tracking... Press 'q' or ESC to quit." << std::endl;

    // Map to store track_id -> class_id for consistent labeling
    std::map<size_t, int> track_class_map;

    cv::Mat frame;
    while (true)
    {
        // Capture frame from webcam
        cap >> frame;

        if (frame.empty())
        {
            std::cerr << "Error: Empty frame captured!" << std::endl;
            break;
        }

        // YOLO Detection
        std::vector<Detection> detections = inf.runInference(frame);

        // Convert YOLO detections to ByteTrack objects
        std::vector<byte_track::Object> objects = convertToByteTrackObjects(detections);

        // Update tracker with new detections
        std::vector<byte_track::BYTETracker::STrackPtr> tracks = tracker.update(objects);

        // Match tracks with detections using IoU to update class mapping
        for (const auto& track : tracks)
        {
            const auto& rect = track->getRect();
            size_t track_id = track->getTrackId();

            // Convert ByteTrack Rect to OpenCV Rect
            cv::Rect track_box(
                static_cast<int>(rect.x()),
                static_cast<int>(rect.y()),
                static_cast<int>(rect.width()),
                static_cast<int>(rect.height())
            );

            // Find best matching detection by IoU
            float best_iou = 0.0f;
            int best_class_id = -1;

            for (const auto& det : detections)
            {
                float iou = calcIoU(track_box, det.box);
                if (iou > best_iou && iou > 0.3f) // IoU threshold = 0.3
                {
                    best_iou = iou;
                    best_class_id = det.class_id;
                }
            }

            // Update class mapping if we found a good match
            if (best_class_id >= 0)
            {
                track_class_map[track_id] = best_class_id;
            }
        }

        // Draw tracked objects on frame
        for (const auto& track : tracks)
        {
            const auto& rect = track->getRect();
            size_t track_id = track->getTrackId();
            float score = track->getScore();

            // Convert ByteTrack Rect to OpenCV Rect
            cv::Rect box(
                static_cast<int>(rect.x()),
                static_cast<int>(rect.y()),
                static_cast<int>(rect.width()),
                static_cast<int>(rect.height())
            );

            // Get consistent color for this track ID
            cv::Scalar color = getColorForTrackID(track_id);

            // Draw bounding box
            cv::rectangle(frame, box, color, 2);

            // Get class name from track_class_map
            std::string className = "unknown";
            if (track_class_map.find(track_id) != track_class_map.end())
            {
                int class_id = track_class_map[track_id];
                className = inf.getClassName(class_id);
            }

            // Draw text with track ID
            std::string label = "[ID:" + std::to_string(track_id) + "] " +
                               className + " " +
                               std::to_string(score).substr(0, 4);

            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_DUPLEX, 0.7, 2, 0);
            cv::Rect textBox(box.x, box.y - 35, textSize.width + 10, textSize.height + 20);

            cv::rectangle(frame, textBox, color, cv::FILLED);
            cv::putText(frame, label, cv::Point(box.x + 5, box.y - 10),
                       cv::FONT_HERSHEY_DUPLEX, 0.7, cv::Scalar(255, 255, 255), 2, 0);
        }

        // Display tracking info
        std::string infoText = "Tracks: " + std::to_string(tracks.size()) +
                              " | Detections: " + std::to_string(detections.size());
        cv::putText(frame, infoText, cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);

        // Show result
        cv::imshow("YOLOv8 + ByteTrack Webcam Tracking", frame);

        // Exit on 'q' or ESC key
        int key = cv::waitKey(1);
        if (key == 'q' || key == 27) // 27 = ESC
        {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();

    std::cout << "Inference stopped." << std::endl;
    return 0;
}