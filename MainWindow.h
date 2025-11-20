#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QGridLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <memory>
#include <vector>

#include "CameraManager.h"
#include "CameraWidget.h"
#include "inference.h"
#include "GridManager.h"
#include "CameraGridWidget.h"
#include "CropsPanelWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onAddCamera();
    void onRemoveCamera(int cameraId);
    void onSaveConfiguration();
    void onLoadConfiguration();
    void onStartAll();
    void onStopAll();
    void onAbout();
    void onSelectModel();
    void onEvents();
    void onDisplaySettings();
    void onTelegramSettings();
    void onLoadData();
    void onFullScreenRequested(int cameraId);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void loadCamerasToGrid();  // NEW: Load cameras from manager to CameraGridWidget
    void updateCameraGrid();  // DEPRECATED: Kept for backward compatibility
    void clearCameraGrid();   // DEPRECATED: Kept for backward compatibility
    void loadDisplaySettings();
    void applyDisplaySettings();
    QWidget* createPlaceholderWidget();

    // New grid management slots
    void onGridWidgetAdded(int id, int row, int col);
    void onGridWidgetRemoved(int id);

    // New layout slots
    void onCropDetected(const QPixmap& cropImage, const QPixmap& fullFrameImage,
                        const QString& cameraName, const QString& className,
                        int trackId, float confidence);

    std::unique_ptr<CameraManager> cameraManager_;
    std::shared_ptr<Inference> inference_;
    std::unique_ptr<GridManager> gridManager_;  // DEPRECATED: Old grid manager
    std::map<int, CameraWidget*> cameraWidgetMap_;  // ID -> Widget mapping

    // DEPRECATED: Old vector approach (will be removed)
    std::vector<CameraWidget*> cameraWidgets_;

    // New layout components
    CameraGridWidget* cameraGridWidget_;  // Fixed 2x2 camera grid
    CropsPanelWidget* cropsPanelWidget_;  // Realtime crops panel

    QWidget* centralWidget_;
    QGridLayout* gridLayout_;  // DEPRECATED: Old layout

    // Actions
    QAction* addCameraAction_;
    QAction* selectModelAction_;
    QAction* displaySettingsAction_;
    QAction* telegramSettingsAction_;
    QAction* loadDataAction_;
    QAction* saveConfigAction_;
    QAction* loadConfigAction_;
    QAction* startAllAction_;
    QAction* stopAllAction_;
    QAction* exitAction_;
    QAction* eventsAction_;
    QAction* aboutAction_;

    // Display settings
    int cameraWidth_;
    int cameraHeight_;
    int gridRows_;
    int gridColumns_;

    // Model management
    QString currentModelPath_;
    QLabel* modelNameLabel_;
    void updateModelNameLabel();

    static constexpr const char* CONFIG_FILE = "cameras_config.json";
};

#endif // MAINWINDOW_H
