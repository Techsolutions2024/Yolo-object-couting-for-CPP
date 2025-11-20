#include "CameraWidget.h"
#include "yolo_to_bytetrack.h"
#include "RegionManagerDialog.h"
#include "RegionCountManager.h"
#include "ClassFilterManager.h"
#include <QMessageBox>
#include <QGroupBox>
#include <QInputDialog>
#include <set>

CameraWidget::CameraWidget(std::shared_ptr<CameraSource> camera,
                          std::shared_ptr<Inference> inference,
                          QWidget* parent)
    : QWidget(parent), camera_(camera), inference_(inference), isRunning_(false), currentFrameNumber_(0) {

    // Initialize ByteTrack tracker
    // Parameters: frame_rate, track_buffer, track_thresh, high_thresh, match_thresh
    tracker_ = std::make_unique<byte_track::BYTETracker>(30, 30, 0.5, 0.6, 0.8);

    // Store camera name for overlay
    cameraName_ = QString::fromStdString(camera_->getName());

    setupUI();

    // Setup timer for frame updates
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &CameraWidget::updateFrame);
}

CameraWidget::~CameraWidget() {
    stopCapture();
}

void CameraWidget::setupUI() {
    // Create layout with minimal margins for clean appearance
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Video display label (RegionDrawingWidget)
    videoLabel_ = new RegionDrawingWidget();

    // NEW: Responsive layout - widget will fill available space
    // Set minimum size to ensure usability, but allow expansion
    videoLabel_->setMinimumSize(320, 240);  // Minimum usable size
    videoLabel_->setScaledContents(true);
    videoLabel_->setStyleSheet(
        "QLabel { "
        "background-color: black; "
        "border: 1px solid #333; "
        "}"
    );
    videoLabel_->setAlignment(Qt::AlignCenter);
    connect(videoLabel_, &RegionDrawingWidget::regionCompleted,
            this, &CameraWidget::onRegionCompleted);

    // NEW: Enable expanding size policy for responsive layout
    videoLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addWidget(videoLabel_);
    setLayout(mainLayout);

    // NEW: Allow widget to expand and fill grid cell
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Create hidden UI elements for compatibility (will be accessed via context menu)
    infoLabel_ = nullptr;  // Not displayed, info shown as overlay
    startStopButton_ = nullptr;  // Control via context menu
    removeButton_ = nullptr;  // Control via context menu
}

void CameraWidget::startCapture() {
    if (isRunning_) return;

    if (!camera_->isOpened()) {
        if (!camera_->open()) {
            QMessageBox::critical(this, "Error",
                QString("Failed to open camera: %1").arg(cameraName_));
            return;
        }
    }

    isRunning_ = true;
    timer_->start(33); // ~30 FPS
}

void CameraWidget::stopCapture() {
    if (!isRunning_) return;

    isRunning_ = false;
    timer_->stop();
    camera_->close();
    videoLabel_->clear();
    videoLabel_->setText("Camera Stopped");
}

void CameraWidget::toggleCapture() {
    if (isRunning_) {
        stopCapture();
    } else {
        startCapture();
    }
}

void CameraWidget::updateFrame() {
    if (!camera_->read(currentFrame_) || currentFrame_.empty()) {
        stopCapture();
        return;
    }

    currentFrameNumber_++;
    processFrame(currentFrame_);

    // Convert to QImage and display
    QImage qimg = cvMatToQImage(currentFrame_);
    videoLabel_->setPixmap(QPixmap::fromImage(qimg));
}

void CameraWidget::processFrame(cv::Mat& frame) {
    // Draw regions overlay first
    drawRegionsOnFrame(frame);

    // YOLO Detection
    std::vector<Detection> detections = inference_->runInference(frame);

    // STEP 1: Filter detections by SELECTED CLASSES (if any)
    std::vector<Detection> classFilteredDetections;
    int originalCount = static_cast<int>(detections.size());

    for (const auto& det : detections) {
        // Check if this class should be detected/counted
        if (ClassFilterManager::getInstance().shouldCountClass(det.class_id)) {
            classFilteredDetections.push_back(det);
        }
    }

    // Debug logging (only log when filtering actually happens)
    if (!ClassFilterManager::getInstance().isCountAllMode() && originalCount > 0) {
        static int logCounter = 0;
        if (logCounter++ % 100 == 0) {  // Log every 100 frames to avoid spam
            std::cout << "ClassFilter: " << originalCount << " detections → "
                     << classFilteredDetections.size() << " after class filtering" << std::endl;
        }
    }

    // STEP 2: Filter detections based on regions (if regions are defined)
    std::vector<Detection> filteredDetections;
    std::map<size_t, std::string> detectionRegionMap;  // detection index -> region name

    if (!regions_.empty()) {
        // Only keep detections inside defined regions
        for (size_t i = 0; i < classFilteredDetections.size(); ++i) {
            const auto& det = classFilteredDetections[i];
            bool inRegion = false;
            std::string regionName;

            for (const auto& region : regions_) {
                if (region.containsRect(det.box)) {
                    inRegion = true;
                    regionName = region.getName();
                    break;
                }
            }

            if (inRegion) {
                filteredDetections.push_back(det);
                detectionRegionMap[filteredDetections.size() - 1] = regionName;
            }
        }
    } else {
        // No regions defined, use all class-filtered detections
        filteredDetections = classFilteredDetections;
    }

    // Convert filtered YOLO detections to ByteTrack objects
    std::vector<byte_track::Object> objects = convertToByteTrackObjects(filteredDetections);

    // Update tracker with filtered detections
    std::vector<byte_track::BYTETracker::STrackPtr> tracks = tracker_->update(objects);

    // Match tracks with detections using IoU to update class mapping
    for (const auto& track : tracks) {
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

        for (const auto& det : filteredDetections) {
            float iou = calcIoU(track_box, det.box);
            if (iou > best_iou && iou > 0.3f) {
                best_iou = iou;
                best_class_id = det.class_id;
            }
        }

        // Update class mapping if we found a good match
        if (best_class_id >= 0) {
            trackClassMap_[track_id] = best_class_id;
        }
    }

    // Draw tracked objects on frame
    for (const auto& track : tracks) {
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
        if (trackClassMap_.find(track_id) != trackClassMap_.end()) {
            int class_id = trackClassMap_[track_id];
            className = inference_->getClassName(class_id);
        }

        // Find region name for this detection
        std::string regionName;
        bool isInRegion = false;
        if (!regions_.empty()) {
            for (const auto& region : regions_) {
                if (region.containsRect(box)) {
                    regionName = region.getName();
                    isInRegion = true;
                    break;
                }
            }
        }

        // Event capture logic
        if (!regionName.empty()) {
            // Track unique object in region
            // Note: Class filtering already done in detection phase,
            // so all objects here are already filtered by selected classes
            regionUniqueObjectIds_[regionName].insert(track_id);

            auto& state = trackRegionStates_[track_id];

            if (!state.inRegion) {
                // Object just entered region - FIRST_ENTRY event
                captureEvent(track_id, regionName, box, EventType::FIRST_ENTRY);
                state.inRegion = true;
                state.regionName = regionName;
                state.entryFrame = currentFrameNumber_;
                state.lastCaptureFrame = currentFrameNumber_;

                // Record unique object entry for region counting
                bool isNewUniqueId = RegionCountManager::getInstance().recordObjectEntry(
                    regionName, track_id, cameraName_.toStdString()
                );

                // Optional: Log when a new unique object is counted
                if (isNewUniqueId) {
                    std::cout << "[RegionCount] New object ID " << track_id
                              << " entered region '" << regionName
                              << "' (Camera: " << cameraName_.toStdString() << ")"
                              << " - Total unique count: "
                              << RegionCountManager::getInstance().getRegionCount(regionName)
                              << std::endl;
                }
            } else {
                // Object still in region - check for PERIODIC event
                int framesSinceLastCapture = currentFrameNumber_ - state.lastCaptureFrame;
                int captureInterval = EventManager::getInstance().getPeriodicCaptureInterval();

                if (framesSinceLastCapture >= captureInterval) {
                    captureEvent(track_id, regionName, box, EventType::PERIODIC);
                    state.lastCaptureFrame = currentFrameNumber_;
                }
            }
        } else {
            // Check if object left region - EXIT event
            auto it = trackRegionStates_.find(track_id);
            if (it != trackRegionStates_.end() && it->second.inRegion) {
                captureEvent(track_id, it->second.regionName, box, EventType::EXIT);
                trackRegionStates_.erase(it);
            }
        }

        // Emit crop for realtime display panel (throttled to avoid flooding)
        // Only emit every N frames to reduce overhead
        static std::map<size_t, int> lastEmitFrame;
        const int EMIT_INTERVAL = 30; // Emit once per second at 30fps

        if (lastEmitFrame[track_id] == 0 ||
            (currentFrameNumber_ - lastEmitFrame[track_id]) >= EMIT_INTERVAL) {

            // Crop the detection from the frame
            cv::Rect safeBox = box & cv::Rect(0, 0, frame.cols, frame.rows);
            if (safeBox.width > 0 && safeBox.height > 0) {
                cv::Mat cropMat = frame(safeBox).clone();
                QImage cropImage = cvMatToQImage(cropMat);
                QPixmap cropPixmap = QPixmap::fromImage(cropImage);

                // Convert full frame (with all detections drawn) to QPixmap
                QImage fullFrameImage = cvMatToQImage(frame);
                QPixmap fullFramePixmap = QPixmap::fromImage(fullFrameImage);

                emit cropDetected(cropPixmap,
                                fullFramePixmap,
                                cameraName_,
                                QString::fromStdString(className),
                                static_cast<int>(track_id),
                                score);

                lastEmitFrame[track_id] = currentFrameNumber_;
            }
        }

        // Draw text with track ID and region name
        std::string label = "[ID:" + std::to_string(track_id) + "] " +
                           className + " " +
                           std::to_string(score).substr(0, 4);

        if (!regionName.empty()) {
            label += " [" + regionName + "]";
        }

        cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_DUPLEX, 0.6, 2, 0);
        cv::Rect textBox(box.x, box.y - 30, textSize.width + 10, textSize.height + 15);

        cv::rectangle(frame, textBox, color, cv::FILLED);
        cv::putText(frame, label, cv::Point(box.x + 5, box.y - 8),
                   cv::FONT_HERSHEY_DUPLEX, 0.6, cv::Scalar(255, 255, 255), 2, 0);
    }

    // Draw camera name overlay (top-left corner)
    std::string cameraNameText = cameraName_.toStdString();
    cv::Size nameSize = cv::getTextSize(cameraNameText, cv::FONT_HERSHEY_DUPLEX, 0.8, 2, 0);
    cv::Rect nameBox(5, 5, nameSize.width + 15, nameSize.height + 15);
    cv::rectangle(frame, nameBox, cv::Scalar(0, 0, 0), cv::FILLED);
    cv::putText(frame, cameraNameText, cv::Point(12, 25),
               cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);

    // Draw tracking info overlay (bottom-left corner)
    std::string infoText = "Tracks: " + std::to_string(tracks.size()) +
                          " | Detections: " + std::to_string(filteredDetections.size()) +
                          " | Regions: " + std::to_string(regions_.size());

    // Add class filter info
    if (!ClassFilterManager::getInstance().isCountAllMode()) {
        int selectedCount = ClassFilterManager::getInstance().getSelectedClassCount();
        infoText += " | Filter: " + std::to_string(selectedCount) + " classes";
    } else {
        infoText += " | Filter: ALL";
    }

    std::string statusText = isRunning_ ? "Running" : "Stopped";
    std::string fullInfoText = statusText + " | " + infoText;

    cv::Size infoSize = cv::getTextSize(fullInfoText, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, 0);
    int infoY = frame.rows - 10;
    cv::Rect infoBox(5, infoY - infoSize.height - 10, infoSize.width + 15, infoSize.height + 15);
    cv::rectangle(frame, infoBox, cv::Scalar(0, 0, 0), cv::FILLED);

    // Use different color for filtered mode
    cv::Scalar infoColor = ClassFilterManager::getInstance().isCountAllMode() ?
                          cv::Scalar(0, 255, 0) : cv::Scalar(0, 165, 255);  // Green for all, Orange for filtered

    cv::putText(frame, fullInfoText, cv::Point(12, infoY - 5),
               cv::FONT_HERSHEY_SIMPLEX, 0.5, infoColor, 1, cv::LINE_AA);
}

void CameraWidget::drawRegionsOnFrame(cv::Mat& frame) {
    if (regions_.empty()) {
        return;
    }

    // Set image size for the region drawing widget
    videoLabel_->setImageSize(QSize(frame.cols, frame.rows));

    // Draw each region
    for (const auto& region : regions_) {
        const auto& points = region.getPoints();
        if (points.size() < 3) {
            continue;  // Skip invalid regions
        }

        // Draw filled polygon with transparency
        cv::Mat overlay = frame.clone();
        std::vector<std::vector<cv::Point>> polygons = {points};
        cv::fillPoly(overlay, polygons, region.getColor());
        cv::addWeighted(overlay, 0.3, frame, 0.7, 0, frame);

        // Draw polygon border
        cv::polylines(frame, polygons, true, region.getColor(), 2, cv::LINE_AA);

        // Draw unique object count at centroid (instead of region name)
        if (!points.empty()) {
            cv::Point centroid(0, 0);
            for (const auto& pt : points) {
                centroid.x += pt.x;
                centroid.y += pt.y;
            }
            centroid.x /= points.size();
            centroid.y /= points.size();

            // Get unique object count for this region
            std::string regionName = region.getName();
            int uniqueCount = 0;
            auto it = regionUniqueObjectIds_.find(regionName);
            if (it != regionUniqueObjectIds_.end()) {
                uniqueCount = static_cast<int>(it->second.size());
            }

            // Display count instead of name
            std::string regionLabel = std::to_string(uniqueCount);
            int fontFace = cv::FONT_HERSHEY_DUPLEX;
            double fontScale = 1.2;
            int thickness = 2;

            cv::Size textSize = cv::getTextSize(regionLabel, fontFace, fontScale, thickness, 0);
            cv::Point textOrg(centroid.x - textSize.width / 2, centroid.y + textSize.height / 2);

            // Draw text background (larger for better visibility)
            cv::Rect textRect(textOrg.x - 10, textOrg.y - textSize.height - 10,
                             textSize.width + 20, textSize.height + 20);
            cv::rectangle(frame, textRect, region.getColor(), cv::FILLED);

            // Draw border around count
            cv::rectangle(frame, textRect, cv::Scalar(255, 255, 255), 2);

            // Draw count text
            cv::putText(frame, regionLabel, textOrg, fontFace, fontScale,
                       cv::Scalar(255, 255, 255), thickness, cv::LINE_AA);
        }
    }
}

QImage CameraWidget::cvMatToQImage(const cv::Mat& mat) {
    if (mat.empty()) {
        return QImage();
    }

    cv::Mat rgb;
    if (mat.channels() == 1) {
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    } else {
        rgb = mat.clone();
    }

    return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
}

void CameraWidget::updateInference(std::shared_ptr<Inference> inference) {
    inference_ = inference;
    // Clear track class map since the new model might have different classes
    trackClassMap_.clear();
}

void CameraWidget::setDisplaySize(int width, int height) {
    videoLabel_->setFixedSize(width, height);
    setFixedSize(width, height);

    // Ensure fixed size policy
    videoLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void CameraWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu contextMenu(this);

    // Camera control actions
    QAction* startStopAction = contextMenu.addAction(isRunning_ ? "Stop Camera" : "Start Camera");
    QAction* fullScreenAction = contextMenu.addAction("View Full Screen");
    QAction* removeAction = contextMenu.addAction("Remove Camera");

    contextMenu.addSeparator();

    // Region actions
    QAction* drawRegionAction = contextMenu.addAction("Draw Region");
    QAction* manageRegionsAction = contextMenu.addAction("Manage Regions");

    contextMenu.addSeparator();

    // Info display
    QString cameraInfo = QString("%1 | Regions: %2").arg(cameraName_).arg(regions_.size());
    QAction* infoAction = contextMenu.addAction(cameraInfo);
    infoAction->setEnabled(false);

    QAction* selectedAction = contextMenu.exec(event->globalPos());

    if (selectedAction == startStopAction) {
        toggleCapture();
    } else if (selectedAction == fullScreenAction) {
        emit requestFullScreen(getCameraId());
    } else if (selectedAction == removeAction) {
        onRemoveClicked();
    } else if (selectedAction == drawRegionAction) {
        onDrawRegion();
    } else if (selectedAction == manageRegionsAction) {
        onManageRegions();
    }
}

void CameraWidget::onDrawRegion() {
    if (videoLabel_->isDrawing()) {
        QMessageBox::warning(this, "Drawing in Progress",
            "Please complete the current region before starting a new one.");
        return;
    }

    videoLabel_->setEnabled(true);
}

void CameraWidget::onManageRegions() {
    RegionManagerDialog dialog(regions_, this);
    dialog.exec();

    // Regions vector is modified by reference in the dialog
    // Force a repaint to show updated regions
    update();
}

void CameraWidget::onRegionCompleted(const std::vector<cv::Point>& points) {
    bool ok;
    QString name = QInputDialog::getText(
        this,
        "Name Region",
        "Enter a name for this region:",
        QLineEdit::Normal,
        QString("Region %1").arg(regions_.size() + 1),
        &ok
    );

    if (ok && !name.isEmpty()) {
        Region region(name.toStdString(), points);
        regions_.push_back(region);

        QMessageBox::information(this, "Region Added",
            QString("Region '%1' has been added with %2 points.")
                .arg(name)
                .arg(points.size()));
    }

    videoLabel_->setEnabled(false);
}

void CameraWidget::captureEvent(size_t trackId, const std::string& regionName,
                                 const cv::Rect& bbox, EventType eventType) {
    // Get object class
    std::string objectClass = "unknown";
    float confidence = 0.0f;

    if (trackClassMap_.find(trackId) != trackClassMap_.end()) {
        int class_id = trackClassMap_[trackId];
        objectClass = inference_->getClassName(class_id);
        confidence = 0.85f;  // Default confidence (could be improved to store actual value)
    }

    // Crop object from current frame
    cv::Mat croppedImage;
    if (!currentFrame_.empty() && bbox.x >= 0 && bbox.y >= 0 &&
        bbox.x + bbox.width <= currentFrame_.cols &&
        bbox.y + bbox.height <= currentFrame_.rows) {

        // Add some padding around the object
        int padding = 15;
        int x = std::max(0, bbox.x - padding);
        int y = std::max(0, bbox.y - padding);
        int w = std::min(currentFrame_.cols - x, bbox.width + 2 * padding);
        int h = std::min(currentFrame_.rows - y, bbox.height + 2 * padding);

        cv::Rect paddedBox(x, y, w, h);
        croppedImage = currentFrame_(paddedBox).clone();
    }

    if (croppedImage.empty()) {
        return;  // Failed to crop, skip event
    }

    // Save image to disk
    EventManager& manager = EventManager::getInstance();
    std::string imagePath = manager.saveEventImage(
        croppedImage,
        camera_->getName(),
        regionName,
        trackId,
        eventType
    );

    if (imagePath.empty()) {
        return;  // Failed to save, skip event
    }

    // Create and add event
    DetectionEvent event(
        trackId,
        camera_->getId(),
        camera_->getName(),
        regionName,
        objectClass,
        confidence,
        eventType,
        bbox,
        imagePath
    );

    event.setFrameNumber(currentFrameNumber_);
    manager.addEvent(event);

    // Send to Telegram if enabled and event is FIRST_ENTRY or EXIT
    if (TelegramBot::getInstance().isEnabled() &&
        (eventType == EventType::FIRST_ENTRY || eventType == EventType::EXIT)) {

        // Check throttling - only send if 5 seconds passed since last send for this region
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        auto it = lastTelegramSendTime_.find(regionName);

        bool canSend = (it == lastTelegramSendTime_.end() ||
                        (currentTime - it->second) >= TELEGRAM_THROTTLE_MS);

        if (canSend) {
            // Get region count
            int regionCount = RegionCountManager::getInstance().getRegionCount(regionName);

            // Build caption
            QString eventTypeStr = (eventType == EventType::FIRST_ENTRY) ? "ENTRY" : "EXIT";
            QString caption = QString("[%1] Camera: %2 | Region: %3 | Count: %4")
                .arg(eventTypeStr)
                .arg(QString::fromStdString(camera_->getName()))
                .arg(QString::fromStdString(regionName))
                .arg(regionCount);

            // Draw bounding box on full frame
            cv::Mat fullFrameWithBox = currentFrame_.clone();
            cv::rectangle(fullFrameWithBox, bbox, cv::Scalar(0, 255, 0), 2);

            // Add label above bounding box
            std::string label = objectClass + " ID:" + std::to_string(trackId);
            int baseline;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 2, &baseline);
            cv::rectangle(fullFrameWithBox,
                         cv::Point(bbox.x, bbox.y - textSize.height - 5),
                         cv::Point(bbox.x + textSize.width, bbox.y),
                         cv::Scalar(0, 255, 0), -1);
            cv::putText(fullFrameWithBox, label,
                       cv::Point(bbox.x, bbox.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(0, 0, 0), 2);

            // Convert to QPixmap
            cv::Mat rgb;
            cv::cvtColor(fullFrameWithBox, rgb, cv::COLOR_BGR2RGB);
            QImage qimg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
            QPixmap pixmap = QPixmap::fromImage(qimg.copy());

            // Send to Telegram
            TelegramBot::getInstance().sendPhoto(pixmap, caption);

            // Update throttle time
            lastTelegramSendTime_[regionName] = currentTime;

            std::cout << "📤 Telegram: Sent " << eventTypeStr.toStdString()
                      << " event for region '" << regionName << "'" << std::endl;
        } else {
            // Throttled
            qint64 timeSinceLastSend = currentTime - it->second;
            qint64 timeRemaining = TELEGRAM_THROTTLE_MS - timeSinceLastSend;
            std::cout << "⏱️  Telegram: Throttled for region '" << regionName
                      << "' (wait " << (timeRemaining / 1000) << "s)" << std::endl;
        }
    }
}

void CameraWidget::onRemoveClicked() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Remove",
                                  QString("Are you sure you want to remove camera '%1'?")
                                      .arg(QString::fromStdString(camera_->getName())),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        stopCapture();
        emit cameraRemoved(camera_->getId());
    }
}
