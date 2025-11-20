#include "MainWindow.h"
#include "AddCameraDialog.h"
#include "Region.h"
#include "EventsViewerWidget.h"
#include "DisplaySettingsDialog.h"
#include "TelegramSettingsDialog.h"
#include "ClassSelectionDialog.h"
#include "ClassFilterManager.h"
#include "RegionCountManager.h"
#include "EventManager.h"
#include "FullScreenCameraView.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>
#include <chrono>

using json = nlohmann::json;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), modelNameLabel_(nullptr) {

    // Initialize camera manager
    cameraManager_ = std::make_unique<CameraManager>();

    // Load display settings (includes model path)
    loadDisplaySettings();

    // Initialize YOLO inference with saved/default model
    bool runOnGPU = false;

    // Check if model file exists
    if (!QFile::exists(currentModelPath_)) {
        std::cerr << "⚠️  Model file not found: " << currentModelPath_.toStdString() << std::endl;
        std::cerr << "   Falling back to default: yolov8n.onnx" << std::endl;
        currentModelPath_ = "yolov8n.onnx";
    }

    // Try to load the model
    try {
        inference_ = std::make_shared<Inference>(
            currentModelPath_.toStdString(),
            cv::Size(640, 640),
            "classes.txt",
            runOnGPU
        );
        std::cout << "✅ Model loaded: " << currentModelPath_.toStdString() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to load model: " << e.what() << std::endl;
        std::cerr << "   Trying default: yolov8n.onnx" << std::endl;

        // Fallback to default model
        currentModelPath_ = "yolov8n.onnx";
        try {
            inference_ = std::make_shared<Inference>(
                currentModelPath_.toStdString(),
                cv::Size(640, 640),
                "classes.txt",
                runOnGPU
            );
            std::cout << "✅ Default model loaded successfully" << std::endl;
        } catch (const std::exception& fallbackError) {
            std::cerr << "❌ FATAL: Cannot load default model: " << fallbackError.what() << std::endl;
            QMessageBox::critical(nullptr, "Fatal Error",
                QString("Cannot load YOLO model!\n\nError: %1\n\nPlease ensure yolov8n.onnx exists in the application directory.")
                    .arg(fallbackError.what()));
            std::exit(1);
        }
    }

    setupUI();
    setupMenuBar();
    setupToolBar();

    setWindowTitle("YOLOv8 Multi-Camera Tracking System");
    resize(1400, 900);

    // Try to load previous configuration
    if (QFile::exists(CONFIG_FILE)) {
        cameraManager_->loadFromFile(CONFIG_FILE);
        loadCamerasToGrid();  // NEW: Use new method for CameraGridWidget
    }

    // Initialize RegionCountManager with auto-save enabled
    RegionCountManager::getInstance().setAutoSave(true, "region_count.json");

    // Try to load previous region count data if exists
    if (QFile::exists("region_count.json")) {
        bool loaded = RegionCountManager::getInstance().loadFromJson("region_count.json");
        if (loaded) {
            std::cout << "✅ Region count data loaded from region_count.json" << std::endl;
        }
    }

    // Load previous events from events/ directory
    EventManager::getInstance().loadEventsFromDirectory();

    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow() {
    // Auto-save configuration on exit
    cameraManager_->saveToFile(CONFIG_FILE);
}

void MainWindow::setupUI() {
    // Create main central widget with horizontal layout (75% grid / 25% crops panel)
    centralWidget_ = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget_);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Create fixed 2x2 camera grid widget (left side - 75%)
    cameraGridWidget_ = new CameraGridWidget(this);
    mainLayout->addWidget(cameraGridWidget_, 3); // Stretch factor 3

    // Create crops panel widget (right side - 25%)
    cropsPanelWidget_ = new CropsPanelWidget(this);
    cropsPanelWidget_->setMaxCrops(50); // Limit to 50 crops
    mainLayout->addWidget(cropsPanelWidget_, 1); // Stretch factor 1

    // Connect grid signals
    connect(cameraGridWidget_, &CameraGridWidget::cameraAdded,
            this, [this](int cameraId, int row, int col) {
                statusBar()->showMessage(
                    QString("Camera %1 added at position [%2,%3]")
                        .arg(cameraId).arg(row).arg(col), 2000);
            });

    connect(cameraGridWidget_, &CameraGridWidget::gridFull,
            this, [this]() {
                statusBar()->showMessage("Grid is full (2x2 = 4 cameras max)", 3000);
            });

    setCentralWidget(centralWidget_);

    // DEPRECATED: Keep old grid layout for backward compatibility but don't use it
    gridLayout_ = nullptr;
    gridManager_ = nullptr;
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("&File");

    addCameraAction_ = new QAction("&Add Camera", this);
    addCameraAction_->setShortcut(QKeySequence("Ctrl+N"));
    connect(addCameraAction_, &QAction::triggered, this, &MainWindow::onAddCamera);
    fileMenu->addAction(addCameraAction_);

    selectModelAction_ = new QAction("Select &Model", this);
    selectModelAction_->setShortcut(QKeySequence("Ctrl+M"));
    connect(selectModelAction_, &QAction::triggered, this, &MainWindow::onSelectModel);
    fileMenu->addAction(selectModelAction_);

    displaySettingsAction_ = new QAction("Display &Settings", this);
    displaySettingsAction_->setShortcut(QKeySequence("Ctrl+D"));
    connect(displaySettingsAction_, &QAction::triggered, this, &MainWindow::onDisplaySettings);
    fileMenu->addAction(displaySettingsAction_);

    telegramSettingsAction_ = new QAction("&Telegram Settings", this);
    telegramSettingsAction_->setShortcut(QKeySequence("Ctrl+T"));
    connect(telegramSettingsAction_, &QAction::triggered, this, &MainWindow::onTelegramSettings);
    fileMenu->addAction(telegramSettingsAction_);

    loadDataAction_ = new QAction("&Load Data (Select Classes)", this);
    loadDataAction_->setShortcut(QKeySequence("Ctrl+L"));
    connect(loadDataAction_, &QAction::triggered, this, &MainWindow::onLoadData);
    fileMenu->addAction(loadDataAction_);

    fileMenu->addSeparator();

    saveConfigAction_ = new QAction("&Save Configuration", this);
    saveConfigAction_->setShortcut(QKeySequence("Ctrl+S"));
    connect(saveConfigAction_, &QAction::triggered, this, &MainWindow::onSaveConfiguration);
    fileMenu->addAction(saveConfigAction_);

    loadConfigAction_ = new QAction("&Load Configuration", this);
    loadConfigAction_->setShortcut(QKeySequence("Ctrl+O"));
    connect(loadConfigAction_, &QAction::triggered, this, &MainWindow::onLoadConfiguration);
    fileMenu->addAction(loadConfigAction_);

    fileMenu->addSeparator();

    exitAction_ = new QAction("E&xit", this);
    exitAction_->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction_, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction_);

    QMenu* controlMenu = menuBar()->addMenu("&Control");

    startAllAction_ = new QAction("Start &All Cameras", this);
    connect(startAllAction_, &QAction::triggered, this, &MainWindow::onStartAll);
    controlMenu->addAction(startAllAction_);

    stopAllAction_ = new QAction("St&op All Cameras", this);
    connect(stopAllAction_, &QAction::triggered, this, &MainWindow::onStopAll);
    controlMenu->addAction(stopAllAction_);

    QMenu* eventsMenu = menuBar()->addMenu("&Events");

    eventsAction_ = new QAction("View &Events", this);
    connect(eventsAction_, &QAction::triggered, this, &MainWindow::onEvents);
    eventsMenu->addAction(eventsAction_);

    QMenu* helpMenu = menuBar()->addMenu("&Help");

    aboutAction_ = new QAction("&About", this);
    connect(aboutAction_, &QAction::triggered, this, &MainWindow::onAbout);
    helpMenu->addAction(aboutAction_);
}

void MainWindow::setupToolBar() {
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);

    toolbar->addAction(addCameraAction_);
    toolbar->addAction(selectModelAction_);
    toolbar->addAction(displaySettingsAction_);
    toolbar->addAction(loadDataAction_);
    toolbar->addSeparator();
    toolbar->addAction(startAllAction_);
    toolbar->addAction(stopAllAction_);
    toolbar->addSeparator();
    toolbar->addAction(saveConfigAction_);

    // Add model name label to toolbar
    toolbar->addSeparator();
    modelNameLabel_ = new QLabel(this);
    updateModelNameLabel();  // Set initial text
    modelNameLabel_->setStyleSheet("QLabel { color: #4CAF50; font-weight: bold; padding: 0 10px; }");
    toolbar->addWidget(modelNameLabel_);
}

void MainWindow::onAddCamera() {
    // Check if grid is full before showing dialog
    if (cameraGridWidget_->isFull()) {
        QMessageBox::warning(this, "Grid Full",
            "Cannot add more cameras. The grid is full (maximum 4 cameras in 2x2 layout).\n"
            "Please remove a camera before adding a new one.");
        return;
    }

    AddCameraDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString name = dialog.getCameraName();
    CameraType type = dialog.getCameraType();
    QString source = dialog.getCameraSource();

    // Performance timing
    auto startTime = std::chrono::high_resolution_clock::now();

    // [1] Add camera to CameraManager
    int cameraId = cameraManager_->addCamera(
        name.toStdString(),
        type,
        source.toStdString()
    );

    // [2] Get camera from manager
    auto cameraPtr = cameraManager_->getAllCameras().back();

    // [3] Create CameraWidget
    auto* cameraWidget = new CameraWidget(cameraPtr, inference_, this);
    connect(cameraWidget, &CameraWidget::cameraRemoved,
            this, &MainWindow::onRemoveCamera);

    // [4] Connect crop detection signal to crops panel
    connect(cameraWidget, &CameraWidget::cropDetected,
            this, &MainWindow::onCropDetected);

    // [5] Add to CameraGridWidget (fixed 2x2 grid)
    if (!cameraGridWidget_->addCamera(cameraWidget, cameraId)) {
        QMessageBox::warning(this, "Grid Full",
            QString("Cannot add camera '%1': Grid is full (2x2 = 4 cameras max).")
                .arg(name));

        // Cleanup
        cameraWidget->deleteLater();
        cameraManager_->removeCamera(cameraId);
        return;
    }

    // [6] Track widget in map
    cameraWidgetMap_[cameraId] = cameraWidget;

    // Performance measurement
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    statusBar()->showMessage(
        QString("Camera '%1' added successfully (took %2ms)")
            .arg(name)
            .arg(duration.count()),
        3000
    );

    std::cout << "MainWindow: Added camera '" << name.toStdString()
              << "' in " << duration.count() << "ms (NEW 2x2 grid method)" << std::endl;
}

void MainWindow::onRemoveCamera(int cameraId) {
    // Performance timing
    auto startTime = std::chrono::high_resolution_clock::now();

    // [1] Find widget in map (O(1))
    auto it = cameraWidgetMap_.find(cameraId);
    if (it == cameraWidgetMap_.end()) {
        std::cerr << "MainWindow: Camera ID " << cameraId << " not found in widget map" << std::endl;
        return;
    }

    CameraWidget* widget = it->second;

    // [2] Stop capture before removal
    if (widget->isRunning()) {
        widget->stopCapture();
    }

    // [3] Remove from CameraGridWidget (fixed 2x2 grid)
    cameraGridWidget_->removeCamera(cameraId);

    // [4] Remove from map
    cameraWidgetMap_.erase(it);

    // [5] Remove from CameraManager
    cameraManager_->removeCamera(cameraId);

    // [6] Delete the widget
    widget->deleteLater();

    // Performance measurement
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    statusBar()->showMessage(
        QString("Camera removed successfully (took %1ms)")
            .arg(duration.count()),
        3000
    );

    std::cout << "MainWindow: Removed camera ID=" << cameraId
              << " in " << duration.count() << "ms (NEW 2x2 grid method)" << std::endl;
}

void MainWindow::onSaveConfiguration() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Save Camera Configuration",
        CONFIG_FILE,
        "JSON Files (*.json);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        // Save cameras
        if (cameraManager_->saveToFile(filename.toStdString())) {
            // Save regions separately
            try {
                json regionsJson;
                regionsJson["regions"] = json::array();

                for (const auto* widget : cameraWidgets_) {
                    json cameraRegions;
                    cameraRegions["camera_id"] = widget->getCameraId();
                    cameraRegions["regions"] = json::array();

                    for (const auto& region : widget->getRegions()) {
                        cameraRegions["regions"].push_back(region.toJson());
                    }

                    regionsJson["regions"].push_back(cameraRegions);
                }

                // Save regions to a companion file
                std::string regionsFilename = filename.toStdString();
                size_t dotPos = regionsFilename.find_last_of('.');
                if (dotPos != std::string::npos) {
                    regionsFilename.insert(dotPos, "_regions");
                } else {
                    regionsFilename += "_regions.json";
                }

                std::ofstream regionsFile(regionsFilename);
                regionsFile << regionsJson.dump(4);
                regionsFile.close();

                QMessageBox::information(this, "Success",
                    "Configuration and regions saved successfully!");
                statusBar()->showMessage("Configuration saved", 3000);
            } catch (const std::exception& e) {
                QMessageBox::warning(this, "Warning",
                    QString("Cameras saved but failed to save regions: %1").arg(e.what()));
            }
        } else {
            QMessageBox::critical(this, "Error", "Failed to save configuration!");
        }
    }
}

void MainWindow::onLoadConfiguration() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Load Camera Configuration",
        CONFIG_FILE,
        "JSON Files (*.json);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        // Stop all cameras before loading
        onStopAll();
        clearCameraGrid();

        if (cameraManager_->loadFromFile(filename.toStdString())) {
            // Try to load regions from companion file
            std::map<int, std::vector<Region>> loadedRegions;
            try {
                std::string regionsFilename = filename.toStdString();
                size_t dotPos = regionsFilename.find_last_of('.');
                if (dotPos != std::string::npos) {
                    regionsFilename.insert(dotPos, "_regions");
                } else {
                    regionsFilename += "_regions.json";
                }

                std::ifstream regionsFile(regionsFilename);
                if (regionsFile.is_open()) {
                    json regionsJson;
                    regionsFile >> regionsJson;
                    regionsFile.close();

                    if (regionsJson.contains("regions")) {
                        for (const auto& cameraRegions : regionsJson["regions"]) {
                            int cameraId = cameraRegions["camera_id"].get<int>();
                            std::vector<Region> regions;

                            if (cameraRegions.contains("regions")) {
                                for (const auto& regionJson : cameraRegions["regions"]) {
                                    regions.push_back(Region::fromJson(regionJson));
                                }
                            }

                            loadedRegions[cameraId] = regions;
                        }
                    }
                }
            } catch (const std::exception& e) {
                // Regions file not found or invalid, continue without regions
                statusBar()->showMessage("Loaded cameras without regions", 3000);
            }

            // Create camera widgets and restore regions using fixed grid
            const auto& cameras = cameraManager_->getAllCameras();
            int totalCells = gridRows_ * gridColumns_;
            int cameraIndex = 0;

            // Fill grid row by row
            for (int row = 0; row < gridRows_; ++row) {
                for (int col = 0; col < gridColumns_; ++col) {
                    QWidget* widget = nullptr;

                    if (cameraIndex < cameras.size()) {
                        // Create CameraWidget
                        auto* cameraWidget = new CameraWidget(cameras[cameraIndex], inference_, this);
                        connect(cameraWidget, &CameraWidget::cameraRemoved,
                                this, &MainWindow::onRemoveCamera);
                        cameraWidget->setDisplaySize(cameraWidth_, cameraHeight_);

                        int cameraId = cameras[cameraIndex]->getId();
                        if (loadedRegions.find(cameraId) != loadedRegions.end()) {
                            cameraWidget->setRegions(loadedRegions[cameraId]);
                        }

                        widget = cameraWidget;
                        cameraWidgets_.push_back(cameraWidget);
                        cameraIndex++;
                    } else {
                        // Create placeholder widget for empty cells
                        widget = createPlaceholderWidget();
                    }

                    gridLayout_->addWidget(widget, row, col);
                }
            }

            // Calculate and set total grid size
            int totalWidth = cameraWidth_ * gridColumns_;
            int totalHeight = cameraHeight_ * gridRows_;
            centralWidget_->setFixedSize(totalWidth, totalHeight);

            QMessageBox::information(this, "Success",
                QString("Configuration loaded successfully!\nCameras: %1\nRegions loaded for %2 camera(s)")
                    .arg(cameras.size())
                    .arg(loadedRegions.size()));
            statusBar()->showMessage("Configuration loaded", 3000);
        } else {
            QMessageBox::critical(this, "Error", "Failed to load configuration!");
        }
    }
}

void MainWindow::onStartAll() {
    for (auto* widget : cameraWidgets_) {
        if (!widget->isRunning()) {
            widget->startCapture();
        }
    }
    statusBar()->showMessage("All cameras started", 3000);
}

void MainWindow::onStopAll() {
    for (auto* widget : cameraWidgets_) {
        if (widget->isRunning()) {
            widget->stopCapture();
        }
    }
    statusBar()->showMessage("All cameras stopped", 3000);
}

void MainWindow::onCropDetected(const QPixmap& cropImage, const QPixmap& fullFrameImage,
                                const QString& cameraName, const QString& className,
                                int trackId, float confidence) {
    // Forward both crop and full frame to the crops panel
    cropsPanelWidget_->addCrop(cropImage, fullFrameImage, cameraName, className, trackId, confidence);
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About",
        "YOLOv8 Multi-Camera Tracking System\n\n"
        "Version 1.0\n\n"
        "Features:\n"
        "- Multi-camera support (Webcam, Video File, RTSP, IP Camera)\n"
        "- Real-time object detection with YOLOv8\n"
        "- Object tracking with ByteTrack\n"
        "- Configuration save/load\n\n"
        "Built with Qt, OpenCV, and YOLOv8"
    );
}

void MainWindow::onEvents() {
    EventsViewerWidget* eventsViewer = new EventsViewerWidget(this);
    eventsViewer->exec();
    delete eventsViewer;
}

void MainWindow::loadCamerasToGrid() {
    // Load cameras from CameraManager into the new CameraGridWidget (2x2)
    const auto& cameras = cameraManager_->getAllCameras();

    if (cameras.empty()) {
        std::cout << "MainWindow: No cameras to load" << std::endl;
        return;
    }

    std::cout << "MainWindow: Loading " << cameras.size() << " camera(s) to grid..." << std::endl;

    // Load up to 4 cameras (2x2 grid limit)
    int camerasToLoad = std::min(static_cast<int>(cameras.size()), 4);

    for (int i = 0; i < camerasToLoad; ++i) {
        auto cameraPtr = cameras[i];
        int cameraId = cameraPtr->getId();

        // Create CameraWidget
        auto* cameraWidget = new CameraWidget(cameraPtr, inference_, this);

        // Connect signals
        connect(cameraWidget, &CameraWidget::cameraRemoved,
                this, &MainWindow::onRemoveCamera);
        connect(cameraWidget, &CameraWidget::cropDetected,
                this, &MainWindow::onCropDetected);
        connect(cameraWidget, &CameraWidget::requestFullScreen,
                this, &MainWindow::onFullScreenRequested);

        // Add to grid
        if (cameraGridWidget_->addCamera(cameraWidget, cameraId)) {
            cameraWidgetMap_[cameraId] = cameraWidget;
            std::cout << "MainWindow: Loaded camera ID=" << cameraId
                      << " (" << cameraPtr->getName() << ") to grid" << std::endl;
        } else {
            std::cerr << "MainWindow: Failed to add camera ID=" << cameraId
                      << " to grid (grid full)" << std::endl;
            cameraWidget->deleteLater();
            break;
        }
    }

    if (cameras.size() > 4) {
        QMessageBox::warning(this, "Grid Capacity",
            QString("Only 4 cameras can be displayed in the 2x2 grid.\n"
                    "Loaded first 4 out of %1 cameras from configuration.")
                .arg(cameras.size()));
    }

    statusBar()->showMessage(
        QString("Loaded %1 camera(s) from configuration").arg(camerasToLoad), 3000);
}

void MainWindow::onSelectModel() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Select YOLOv8 Model",
        "",
        "ONNX Model Files (*.onnx);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        try {
            // Stop all cameras before changing the model
            bool hadRunningCameras = false;

            // Use cameraWidgetMap_ (new grid system)
            for (auto& pair : cameraWidgetMap_) {
                if (pair.second->isRunning()) {
                    hadRunningCameras = true;
                    pair.second->stopCapture();
                }
            }

            // Create new inference instance with selected model
            bool runOnGPU = false;
            auto newInference = std::make_shared<Inference>(
                filename.toStdString(),
                cv::Size(640, 640),
                "classes.txt",
                runOnGPU
            );

            // Replace the shared inference instance
            inference_ = newInference;

            // Update all camera widgets with the new inference
            for (auto& pair : cameraWidgetMap_) {
                pair.second->updateInference(inference_);
            }

            // Save model path to settings
            currentModelPath_ = filename;
            QSettings settings("YOLOTracking", "Yolov8CameraGUI");
            settings.setValue("Model/Path", currentModelPath_);

            // Update toolbar label
            updateModelNameLabel();

            // Clear class filter selection since model changed
            ClassFilterManager::getInstance().clearSelection();

            // Get class count from new model
            int classCount = inference_->getClassCount();

            QString message = QString(
                "Model loaded successfully!\n\n"
                "Model: %1\n"
                "Classes: %2\n\n"
                "💡 Tip: Use 'Load Data' to select which classes to detect/count."
            ).arg(filename).arg(classCount);

            QMessageBox::information(this, "Success", message);
            statusBar()->showMessage(
                QString("Model loaded: %1 (%2 classes)").arg(filename).arg(classCount), 5000);

            std::cout << "✅ Model changed to: " << currentModelPath_.toStdString() << std::endl;
            std::cout << "   Model has " << classCount << " classes" << std::endl;

            // Ask user if they want to restart cameras
            if (hadRunningCameras) {
                auto reply = QMessageBox::question(this, "Restart Cameras",
                    "Do you want to restart the cameras that were running?",
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    onStartAll();
                }
            }

        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error",
                QString("Failed to load model!\n\nError: %1").arg(e.what()));
            statusBar()->showMessage("Failed to load model", 3000);
            std::cerr << "❌ Failed to load model: " << e.what() << std::endl;
        }
    }
}

void MainWindow::updateCameraGrid() {
    // Store running states and regions of existing cameras before clearing
    std::map<int, bool> runningStates;
    std::map<int, std::vector<Region>> cameraRegions;
    for (auto* widget : cameraWidgets_) {
        if (CameraWidget* cam = dynamic_cast<CameraWidget*>(widget)) {
            runningStates[cam->getCameraId()] = cam->isRunning();
            cameraRegions[cam->getCameraId()] = cam->getRegions();
        }
    }

    clearCameraGrid();

    const auto& cameras = cameraManager_->getAllCameras();
    int totalCells = gridRows_ * gridColumns_;
    int cameraIndex = 0;

    // Fill grid row by row
    for (int row = 0; row < gridRows_; ++row) {
        for (int col = 0; col < gridColumns_; ++col) {
            QWidget* widget = nullptr;

            if (cameraIndex < cameras.size()) {
                // Create CameraWidget
                auto* cameraWidget = new CameraWidget(cameras[cameraIndex], inference_, this);
                connect(cameraWidget, &CameraWidget::cameraRemoved,
                        this, &MainWindow::onRemoveCamera);
                cameraWidget->setDisplaySize(cameraWidth_, cameraHeight_);

                int cameraId = cameras[cameraIndex]->getId();

                // Restore regions if camera had them
                if (cameraRegions.find(cameraId) != cameraRegions.end()) {
                    cameraWidget->setRegions(cameraRegions[cameraId]);
                }

                // Restore running state if camera was previously running
                if (runningStates.find(cameraId) != runningStates.end() && runningStates[cameraId]) {
                    cameraWidget->startCapture();
                }

                widget = cameraWidget;
                cameraWidgets_.push_back(cameraWidget);
                cameraIndex++;
            } else {
                // Create placeholder widget for empty cells
                widget = createPlaceholderWidget();
            }

            gridLayout_->addWidget(widget, row, col);
        }
    }

    // Set fixed size for the central widget (grid container)
    int totalWidth = cameraWidth_ * gridColumns_;
    int totalHeight = cameraHeight_ * gridRows_;
    centralWidget_->setFixedSize(totalWidth, totalHeight);

    // Update status
    statusBar()->showMessage(
        QString("Grid: %1×%2 | Cameras: %3/%4 | Total: %5×%6")
            .arg(gridRows_)
            .arg(gridColumns_)
            .arg(cameras.size())
            .arg(totalCells)
            .arg(totalWidth)
            .arg(totalHeight),
        5000
    );
}

void MainWindow::clearCameraGrid() {
    // Remove all widgets from grid
    for (auto* widget : cameraWidgets_) {
        gridLayout_->removeWidget(widget);
        widget->deleteLater();
    }
    cameraWidgets_.clear();
}

void MainWindow::loadDisplaySettings() {
    QSettings settings("YOLOTracking", "Yolov8CameraGUI");
    cameraWidth_ = settings.value("Display/CameraWidth", 640).toInt();
    cameraHeight_ = settings.value("Display/CameraHeight", 480).toInt();
    gridRows_ = settings.value("Display/GridRows", 2).toInt();
    gridColumns_ = settings.value("Display/GridColumns", 2).toInt();

    // Load model path (default: yolov8n.onnx)
    currentModelPath_ = settings.value("Model/Path", "yolov8n.onnx").toString();
}

void MainWindow::updateModelNameLabel() {
    if (modelNameLabel_) {
        QFileInfo fileInfo(currentModelPath_);
        QString modelName = fileInfo.fileName();
        modelNameLabel_->setText(QString("Model: %1").arg(modelName));
    }
}

QWidget* MainWindow::createPlaceholderWidget() {
    QLabel* placeholder = new QLabel();
    placeholder->setFixedSize(cameraWidth_, cameraHeight_);
    placeholder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("QLabel { background-color: #2a2a2a; color: #888888; font-size: 18px; }");
    placeholder->setText("Add Camera +");
    return placeholder;
}

void MainWindow::onDisplaySettings() {
    DisplaySettingsDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        // Get new settings
        cameraWidth_ = dialog.getCameraWidth();
        cameraHeight_ = dialog.getCameraHeight();
        gridRows_ = dialog.getGridRows();
        gridColumns_ = dialog.getGridColumns();

        // Apply settings and resize window
        applyDisplaySettings();
    }
}

void MainWindow::onTelegramSettings() {
    TelegramSettingsDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        statusBar()->showMessage("Telegram settings saved successfully", 3000);
    }
}

void MainWindow::onLoadData() {
    if (!inference_) {
        QMessageBox::warning(this, "Error", "No model loaded!");
        return;
    }

    // Get all classes from model
    const std::vector<std::string>& allClasses = inference_->getAllClasses();

    // Check if classes are loaded
    if (allClasses.empty()) {
        QMessageBox::warning(this, "Warning",
            "Model classes not yet initialized.\n\n"
            "Please start a camera first to initialize the model,\n"
            "then try again.");
        return;
    }

    // Get current selection
    std::set<int> currentSelection = ClassFilterManager::getInstance().getSelectedClasses();

    // Show dialog
    ClassSelectionDialog dialog(allClasses, currentSelection, this);

    if (dialog.exec() == QDialog::Accepted) {
        // Update filter manager
        std::set<int> selectedClasses = dialog.getSelectedClasses();
        ClassFilterManager::getInstance().setSelectedClasses(selectedClasses);

        // Update status bar
        if (dialog.isCountAllMode()) {
            statusBar()->showMessage("Class filter: Counting ALL classes", 5000);
        } else {
            statusBar()->showMessage(
                QString("Class filter: Counting %1 selected class(es)")
                    .arg(selectedClasses.size()), 5000);
        }
    }
}

void MainWindow::onFullScreenRequested(int cameraId) {
    // Find the camera widget
    auto it = cameraWidgetMap_.find(cameraId);
    if (it == cameraWidgetMap_.end()) {
        QMessageBox::warning(this, "Error", "Camera widget not found!");
        return;
    }

    // Get camera from all cameras list
    const auto& cameras = cameraManager_->getAllCameras();
    std::shared_ptr<CameraSource> cameraPtr = nullptr;

    for (const auto& cam : cameras) {
        if (cam->getId() == cameraId) {
            cameraPtr = cam;
            break;
        }
    }

    if (!cameraPtr) {
        QMessageBox::warning(this, "Error", "Camera not found!");
        return;
    }

    // Create and show full screen view
    FullScreenCameraView* fullScreenView = new FullScreenCameraView(cameraPtr, inference_, this);
    fullScreenView->exec();
    delete fullScreenView;

    statusBar()->showMessage(QString("Exited full screen view for camera: %1")
                            .arg(QString::fromStdString(cameraPtr->getName())), 3000);
}

void MainWindow::applyDisplaySettings() {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Check if only size changed (not grid dimensions)
    bool gridSizeChanged = (gridRows_ != gridManager_->getRows() ||
                            gridColumns_ != gridManager_->getColumns());

    if (!gridSizeChanged) {
        // FAST PATH: Only resize existing widgets (O(n))
        std::cout << "MainWindow: Fast path - only resizing cells" << std::endl;
        gridManager_->resizeAllCells(cameraWidth_, cameraHeight_);
    } else {
        // SLOW PATH: Grid dimensions changed, need to rearrange
        std::cout << "MainWindow: Slow path - rearranging grid" << std::endl;

        // Update stretch factors for new dimensions
        for (int row = 0; row < gridRows_; ++row) {
            gridLayout_->setRowStretch(row, 1);
        }
        for (int col = 0; col < gridColumns_; ++col) {
            gridLayout_->setColumnStretch(col, 1);
        }

        // Resize grid
        gridManager_->setGridSize(gridRows_, gridColumns_);
        gridManager_->resizeAllCells(cameraWidth_, cameraHeight_);
    }

    // Calculate total size and resize MainWindow
    int totalWidth = cameraWidth_ * gridColumns_ + 16;  // Add margins
    int totalHeight = cameraHeight_ * gridRows_ + 16;

    // Add space for menubar, toolbar, statusbar
    int extraHeight = menuBar()->height();
    QToolBar* toolbar = findChild<QToolBar*>();
    if (toolbar) {
        extraHeight += toolbar->height();
    }
    extraHeight += statusBar()->height();

    // Resize the main window to fit the grid perfectly
    resize(totalWidth, totalHeight + extraHeight);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    statusBar()->showMessage(
        QString("Display settings applied: Grid %1×%2, Cell %3×%4 (took %5ms)")
            .arg(gridRows_)
            .arg(gridColumns_)
            .arg(cameraWidth_)
            .arg(cameraHeight_)
            .arg(duration.count()),
        5000
    );
}

// NEW: GridManager callback methods
void MainWindow::onGridWidgetAdded(int id, int row, int col) {
    std::cout << "MainWindow: Grid callback - widget " << id
              << " added at (" << row << "," << col << ")" << std::endl;
}

void MainWindow::onGridWidgetRemoved(int id) {
    std::cout << "MainWindow: Grid callback - widget " << id << " removed" << std::endl;
}
