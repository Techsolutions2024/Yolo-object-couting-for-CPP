#include "GridManager.h"
#include <QLabel>
#include <algorithm>
#include <iostream>

GridManager::GridManager(QGridLayout* layout, int rows, int cols, QObject* parent)
    : QObject(parent),
      layout_(layout),
      rows_(rows),
      cols_(cols),
      cellWidth_(640),
      cellHeight_(480),
      placeholderFactory_(nullptr) {

    if (!layout_) {
        throw std::invalid_argument("GridManager: layout cannot be nullptr");
    }

    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("GridManager: rows and cols must be positive");
    }

    // Initialize cells vector
    int totalCells = rows_ * cols_;
    cells_.reserve(totalCells);
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            cells_.emplace_back(row, col);
        }
    }

    // Create initial placeholders
    initializePlaceholders();

    std::cout << "GridManager initialized: " << rows_ << "x" << cols_
              << " (" << totalCells << " cells)" << std::endl;
}

GridManager::~GridManager() {
    clear();
}

bool GridManager::addWidget(QWidget* widget, int id) {
    if (!widget) {
        std::cerr << "GridManager::addWidget: widget is nullptr" << std::endl;
        return false;
    }

    // Check if ID already exists
    if (idToCellIndex_.find(id) != idToCellIndex_.end()) {
        std::cerr << "GridManager::addWidget: widget with ID " << id << " already exists" << std::endl;
        return false;
    }

    // Find first empty cell
    int cellIndex = findFirstEmptyCell();
    if (cellIndex == -1) {
        std::cerr << "GridManager::addWidget: grid is full" << std::endl;
        return false;
    }

    GridCell& cell = cells_[cellIndex];

    // Remove placeholder if exists
    removePlaceholderAt(cellIndex);

    // Add new widget to layout
    layout_->addWidget(widget, cell.row, cell.col);

    // Update cell data
    cell.widget = widget;
    cell.widgetId = id;
    cell.isPlaceholder = false;

    // Update mapping
    idToCellIndex_[id] = cellIndex;

    std::cout << "GridManager: Added widget ID=" << id
              << " at cell (" << cell.row << "," << cell.col << ")" << std::endl;

    emit widgetAdded(id, cell.row, cell.col);
    return true;
}

bool GridManager::removeWidget(int id) {
    auto it = idToCellIndex_.find(id);
    if (it == idToCellIndex_.end()) {
        std::cerr << "GridManager::removeWidget: widget ID " << id << " not found" << std::endl;
        return false;
    }

    int cellIndex = it->second;
    GridCell& cell = cells_[cellIndex];

    // Remove widget from layout
    if (cell.widget) {
        layout_->removeWidget(cell.widget);
        cell.widget->setParent(nullptr);  // Detach from layout
        cell.widget->deleteLater();       // Schedule for deletion
    }

    // Clear cell data
    cell.widget = nullptr;
    cell.widgetId = -1;
    cell.isPlaceholder = false;

    // Remove from mapping
    idToCellIndex_.erase(it);

    // Add placeholder back
    addPlaceholderAt(cellIndex);

    std::cout << "GridManager: Removed widget ID=" << id
              << " from cell (" << cell.row << "," << cell.col << ")" << std::endl;

    emit widgetRemoved(id);
    return true;
}

QWidget* GridManager::getWidget(int id) const {
    auto it = idToCellIndex_.find(id);
    if (it == idToCellIndex_.end()) {
        return nullptr;
    }

    int cellIndex = it->second;
    const GridCell& cell = cells_[cellIndex];

    return cell.isPlaceholder ? nullptr : cell.widget;
}

bool GridManager::hasWidget(int id) const {
    return idToCellIndex_.find(id) != idToCellIndex_.end();
}

void GridManager::setGridSize(int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        std::cerr << "GridManager::setGridSize: invalid dimensions" << std::endl;
        return;
    }

    if (rows == rows_ && cols == cols_) {
        return;  // No change
    }

    std::cout << "GridManager: Resizing grid from " << rows_ << "x" << cols_
              << " to " << rows << "x" << cols << std::endl;

    rows_ = rows;
    cols_ = cols;

    // Rearrange existing widgets
    rearrangeGrid();

    emit gridResized(rows_, cols_);
}

void GridManager::resizeAllCells(int width, int height) {
    if (width <= 0 || height <= 0) {
        std::cerr << "GridManager::resizeAllCells: invalid dimensions" << std::endl;
        return;
    }

    cellWidth_ = width;
    cellHeight_ = height;

    // Resize all non-placeholder widgets
    for (const auto& [id, cellIndex] : idToCellIndex_) {
        GridCell& cell = cells_[cellIndex];
        if (!cell.isPlaceholder && cell.widget) {
            cell.widget->setFixedSize(width, height);
        }
    }

    // Resize placeholders
    for (auto& cell : cells_) {
        if (cell.isPlaceholder && cell.widget) {
            cell.widget->setFixedSize(width, height);
        }
    }

    std::cout << "GridManager: Resized all cells to " << width << "x" << height << std::endl;

    emit cellsResized(width, height);
}

int GridManager::getOccupiedCount() const {
    return static_cast<int>(idToCellIndex_.size());
}

int GridManager::getEmptyCount() const {
    return getTotalCells() - getOccupiedCount();
}

bool GridManager::isFull() const {
    return getEmptyCount() == 0;
}

void GridManager::clear() {
    std::cout << "GridManager: Clearing all widgets" << std::endl;

    // Remove all widgets
    std::vector<int> idsToRemove;
    for (const auto& [id, _] : idToCellIndex_) {
        idsToRemove.push_back(id);
    }

    for (int id : idsToRemove) {
        removeWidget(id);
    }
}

void GridManager::setPlaceholderFactory(PlaceholderFactory factory) {
    placeholderFactory_ = factory;

    // Recreate existing placeholders with new factory
    for (auto& cell : cells_) {
        if (cell.isPlaceholder && cell.widget) {
            layout_->removeWidget(cell.widget);
            cell.widget->deleteLater();

            QWidget* placeholder = createDefaultPlaceholder(cellWidth_, cellHeight_);
            layout_->addWidget(placeholder, cell.row, cell.col);
            cell.widget = placeholder;
        }
    }
}

// Private helper methods

int GridManager::findFirstEmptyCell() const {
    for (size_t i = 0; i < cells_.size(); ++i) {
        if (cells_[i].widgetId == -1) {
            return static_cast<int>(i);
        }
    }
    return -1;  // Grid full
}

QWidget* GridManager::createDefaultPlaceholder(int width, int height) {
    if (placeholderFactory_) {
        return placeholderFactory_(width, height);
    }

    // Default placeholder
    QLabel* placeholder = new QLabel();
    placeholder->setFixedSize(width, height);
    placeholder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet(
        "QLabel { "
        "background-color: #2a2a2a; "
        "color: #888888; "
        "font-size: 18px; "
        "border: 2px dashed #444444; "
        "}"
        "QLabel:hover { "
        "background-color: #353535; "
        "color: #aaaaaa; "
        "}"
    );
    placeholder->setText("Add Camera +");
    placeholder->setCursor(Qt::PointingHandCursor);

    return placeholder;
}

void GridManager::initializePlaceholders() {
    for (auto& cell : cells_) {
        addPlaceholderAt(rowColToIndex(cell.row, cell.col));
    }
}

void GridManager::rearrangeGrid() {
    // Save existing widgets (ID -> widget pointer)
    std::map<int, QWidget*> existingWidgets;
    for (const auto& [id, cellIndex] : idToCellIndex_) {
        GridCell& cell = cells_[cellIndex];
        if (!cell.isPlaceholder && cell.widget) {
            layout_->removeWidget(cell.widget);
            existingWidgets[id] = cell.widget;
        }
    }

    // Clear all cells
    for (auto& cell : cells_) {
        if (cell.widget) {
            if (cell.isPlaceholder) {
                layout_->removeWidget(cell.widget);
                cell.widget->deleteLater();
            }
            cell.widget = nullptr;
        }
        cell.widgetId = -1;
        cell.isPlaceholder = false;
    }
    idToCellIndex_.clear();

    // Resize cells vector
    int newTotalCells = rows_ * cols_;
    cells_.clear();
    cells_.reserve(newTotalCells);
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            cells_.emplace_back(row, col);
        }
    }

    // Re-add existing widgets
    int cellIndex = 0;
    for (const auto& [id, widget] : existingWidgets) {
        if (cellIndex < newTotalCells) {
            GridCell& cell = cells_[cellIndex];
            layout_->addWidget(widget, cell.row, cell.col);

            cell.widget = widget;
            cell.widgetId = id;
            cell.isPlaceholder = false;

            idToCellIndex_[id] = cellIndex;
            cellIndex++;
        } else {
            // Grid too small, delete excess widgets
            std::cerr << "GridManager: Grid too small, removing widget ID=" << id << std::endl;
            widget->deleteLater();
        }
    }

    // Fill remaining cells with placeholders
    for (int i = cellIndex; i < newTotalCells; ++i) {
        addPlaceholderAt(i);
    }

    std::cout << "GridManager: Rearranged grid, " << idToCellIndex_.size()
              << " widgets in " << newTotalCells << " cells" << std::endl;
}

void GridManager::removePlaceholderAt(int cellIndex) {
    if (cellIndex < 0 || cellIndex >= static_cast<int>(cells_.size())) {
        return;
    }

    GridCell& cell = cells_[cellIndex];
    if (cell.isPlaceholder && cell.widget) {
        layout_->removeWidget(cell.widget);
        cell.widget->deleteLater();
        cell.widget = nullptr;
        cell.isPlaceholder = false;
    }
}

void GridManager::addPlaceholderAt(int cellIndex) {
    if (cellIndex < 0 || cellIndex >= static_cast<int>(cells_.size())) {
        return;
    }

    GridCell& cell = cells_[cellIndex];
    if (!cell.isPlaceholder && cell.widgetId == -1) {
        QWidget* placeholder = createDefaultPlaceholder(cellWidth_, cellHeight_);
        layout_->addWidget(placeholder, cell.row, cell.col);

        cell.widget = placeholder;
        cell.isPlaceholder = true;
    }
}
