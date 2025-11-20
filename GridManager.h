#ifndef GRIDMANAGER_H
#define GRIDMANAGER_H

#include <QObject>
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <vector>
#include <map>
#include <memory>

/**
 * @brief GridManager - Manages a grid layout of widgets with incremental updates
 *
 * This class provides efficient O(1) operations for adding/removing widgets
 * from a grid layout, avoiding costly full grid rebuilds.
 *
 * Features:
 * - Incremental add/remove (no rebuild)
 * - ID-based widget tracking
 * - Automatic placeholder management
 * - Grid resizing with state preservation
 */
class GridManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Construct a GridManager
     * @param layout The QGridLayout to manage
     * @param rows Number of grid rows
     * @param cols Number of grid columns
     * @param parent Parent QObject
     */
    explicit GridManager(QGridLayout* layout, int rows, int cols, QObject* parent = nullptr);

    ~GridManager();

    // Widget management
    /**
     * @brief Add a widget to the first available grid cell
     * @param widget The widget to add (GridManager takes ownership)
     * @param id Unique identifier for this widget
     * @return true if added successfully, false if grid is full
     */
    bool addWidget(QWidget* widget, int id);

    /**
     * @brief Remove a widget by its ID
     * @param id The widget ID
     * @return true if removed successfully, false if not found
     */
    bool removeWidget(int id);

    /**
     * @brief Get a widget by its ID
     * @param id The widget ID
     * @return Pointer to widget, or nullptr if not found
     */
    QWidget* getWidget(int id) const;

    /**
     * @brief Check if a widget exists
     * @param id The widget ID
     * @return true if widget exists
     */
    bool hasWidget(int id) const;

    // Grid management
    /**
     * @brief Change grid dimensions (triggers rearrangement)
     * @param rows New number of rows
     * @param cols New number of columns
     */
    void setGridSize(int rows, int cols);

    /**
     * @brief Get current grid dimensions
     */
    int getRows() const { return rows_; }
    int getColumns() const { return cols_; }
    int getTotalCells() const { return rows_ * cols_; }

    // Cell management
    /**
     * @brief Resize all widgets in the grid
     * @param width New cell width
     * @param height New cell height
     */
    void resizeAllCells(int width, int height);

    /**
     * @brief Get number of occupied cells
     */
    int getOccupiedCount() const;

    /**
     * @brief Get number of empty cells
     */
    int getEmptyCount() const;

    /**
     * @brief Check if grid is full
     */
    bool isFull() const;

    /**
     * @brief Clear all widgets from grid
     */
    void clear();

    // Placeholder customization
    /**
     * @brief Set factory function for creating placeholder widgets
     * @param factory Function that creates a placeholder widget
     */
    using PlaceholderFactory = std::function<QWidget*(int width, int height)>;
    void setPlaceholderFactory(PlaceholderFactory factory);

signals:
    /**
     * @brief Emitted when a widget is added to the grid
     * @param id Widget ID
     * @param row Grid row
     * @param col Grid column
     */
    void widgetAdded(int id, int row, int col);

    /**
     * @brief Emitted when a widget is removed from the grid
     * @param id Widget ID
     */
    void widgetRemoved(int id);

    /**
     * @brief Emitted when grid dimensions change
     * @param rows New row count
     * @param cols New column count
     */
    void gridResized(int rows, int cols);

    /**
     * @brief Emitted when all cells are resized
     */
    void cellsResized(int width, int height);

private:
    // Internal data structures
    struct GridCell {
        int row;
        int col;
        int widgetId;        // -1 if empty/placeholder
        QWidget* widget;     // Actual widget or placeholder
        bool isPlaceholder;

        GridCell() : row(-1), col(-1), widgetId(-1), widget(nullptr), isPlaceholder(false) {}
        GridCell(int r, int c) : row(r), col(c), widgetId(-1), widget(nullptr), isPlaceholder(true) {}
    };

    // Helper methods
    int findFirstEmptyCell() const;
    int cellIndexToRow(int index) const { return index / cols_; }
    int cellIndexToCol(int index) const { return index % cols_; }
    int rowColToIndex(int row, int col) const { return row * cols_ + col; }

    QWidget* createDefaultPlaceholder(int width, int height);
    void initializePlaceholders();
    void rearrangeGrid();
    void removePlaceholderAt(int cellIndex);
    void addPlaceholderAt(int cellIndex);

    // Member variables
    QGridLayout* layout_;
    std::vector<GridCell> cells_;
    std::map<int, int> idToCellIndex_;  // Widget ID -> Cell index mapping
    int rows_;
    int cols_;
    int cellWidth_;
    int cellHeight_;

    PlaceholderFactory placeholderFactory_;
};

#endif // GRIDMANAGER_H
