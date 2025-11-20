#ifndef CAMERAGRIDWIDGET_H
#define CAMERAGRIDWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <map>

class CameraWidget;

/**
 * @brief Fixed 2x2 camera grid container widget
 *
 * This widget provides a fixed 2x2 grid layout for displaying up to 4 cameras.
 * Each cell is properly sized and cameras fit within their cells without distortion.
 * The grid maintains its aspect ratio and responds to window resizing.
 */
class CameraGridWidget : public QWidget {
    Q_OBJECT

public:
    explicit CameraGridWidget(QWidget* parent = nullptr);
    ~CameraGridWidget() override;

    /**
     * @brief Add a camera widget to the next available cell
     * @param cameraWidget The camera widget to add
     * @param cameraId The unique ID for this camera
     * @return true if added successfully, false if grid is full
     */
    bool addCamera(CameraWidget* cameraWidget, int cameraId);

    /**
     * @brief Remove a camera widget by ID
     * @param cameraId The camera ID to remove
     * @return true if removed successfully, false if not found
     */
    bool removeCamera(int cameraId);

    /**
     * @brief Get a camera widget by ID
     * @param cameraId The camera ID
     * @return Pointer to camera widget or nullptr if not found
     */
    CameraWidget* getCamera(int cameraId) const;

    /**
     * @brief Check if the grid is full (all 4 slots occupied)
     */
    bool isFull() const;

    /**
     * @brief Check if the grid is empty (no cameras)
     */
    bool isEmpty() const;

    /**
     * @brief Get the number of cameras currently in the grid
     */
    int getCameraCount() const;

    /**
     * @brief Get the maximum number of cameras supported (always 4 for 2x2)
     */
    int getMaxCameras() const { return 4; }

    /**
     * @brief Clear all cameras from the grid
     */
    void clearAllCameras();

    /**
     * @brief Get all camera IDs currently in the grid
     */
    std::vector<int> getCameraIds() const;

signals:
    /**
     * @brief Emitted when a camera is added to the grid
     */
    void cameraAdded(int cameraId, int row, int col);

    /**
     * @brief Emitted when a camera is removed from the grid
     */
    void cameraRemoved(int cameraId);

    /**
     * @brief Emitted when the grid becomes full
     */
    void gridFull();

    /**
     * @brief Emitted when the grid has space available
     */
    void gridHasSpace();

private:
    struct GridCell {
        int row;
        int col;
        CameraWidget* widget;
        QLabel* placeholder;
        bool isEmpty;

        GridCell() : row(-1), col(-1), widget(nullptr), placeholder(nullptr), isEmpty(true) {}
        GridCell(int r, int c) : row(r), col(c), widget(nullptr), placeholder(nullptr), isEmpty(true) {}
    };

    void setupUI();
    void initializeGrid();
    QLabel* createPlaceholder();
    int getNextAvailableCell() const;
    void updateGridState();

    // UI Components
    QGridLayout* gridLayout_;

    // Grid cells (fixed 2x2 = 4 cells)
    static const int GRID_ROWS = 2;
    static const int GRID_COLS = 2;
    GridCell cells_[GRID_ROWS][GRID_COLS];

    // Mapping camera ID to cell position
    std::map<int, std::pair<int, int>> cameraIdToPosition_;

    // Styling
    static const QString GRID_STYLE;
    static const QString PLACEHOLDER_STYLE;
};

#endif // CAMERAGRIDWIDGET_H
