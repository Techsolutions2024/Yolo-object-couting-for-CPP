#include "CameraGridWidget.h"
#include "CameraWidget.h"
#include <algorithm>

// Static style definitions
const QString CameraGridWidget::GRID_STYLE =
    "QWidget { "
    "   background-color: #000000; "
    "}";

const QString CameraGridWidget::PLACEHOLDER_STYLE =
    "QLabel { "
    "   background-color: #1a1a1a; "
    "   border: 2px solid #333333; "
    "   color: #666666; "
    "   font-size: 14px; "
    "   qproperty-alignment: AlignCenter; "
    "}";

CameraGridWidget::CameraGridWidget(QWidget* parent)
    : QWidget(parent), gridLayout_(nullptr) {
    setupUI();
    initializeGrid();
}

CameraGridWidget::~CameraGridWidget() {
    clearAllCameras();
}

void CameraGridWidget::setupUI() {
    setStyleSheet(GRID_STYLE);

    // Create grid layout with fixed 2x2
    gridLayout_ = new QGridLayout(this);
    gridLayout_->setContentsMargins(1, 1, 1, 1);
    gridLayout_->setSpacing(2); // Thin white border between cells

    // Set equal stretch for rows and columns
    for (int row = 0; row < GRID_ROWS; ++row) {
        gridLayout_->setRowStretch(row, 1);
    }
    for (int col = 0; col < GRID_COLS; ++col) {
        gridLayout_->setColumnStretch(col, 1);
    }

    setLayout(gridLayout_);
}

void CameraGridWidget::initializeGrid() {
    // Initialize all 4 cells (2x2)
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            cells_[row][col] = GridCell(row, col);

            // Create and add placeholder
            QLabel* placeholder = createPlaceholder();
            cells_[row][col].placeholder = placeholder;
            gridLayout_->addWidget(placeholder, row, col);
        }
    }
}

QLabel* CameraGridWidget::createPlaceholder() {
    QLabel* placeholder = new QLabel("Camera Stopped");
    placeholder->setStyleSheet(PLACEHOLDER_STYLE);
    placeholder->setMinimumSize(320, 240); // Minimum size for visibility
    placeholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return placeholder;
}

bool CameraGridWidget::addCamera(CameraWidget* cameraWidget, int cameraId) {
    if (!cameraWidget) {
        return false;
    }

    // Check if camera ID already exists
    if (cameraIdToPosition_.find(cameraId) != cameraIdToPosition_.end()) {
        return false; // Already added
    }

    // Find next available cell
    int cellIndex = getNextAvailableCell();
    if (cellIndex == -1) {
        return false; // Grid is full
    }

    int row = cellIndex / GRID_COLS;
    int col = cellIndex % GRID_COLS;

    GridCell& cell = cells_[row][col];

    // Hide placeholder
    if (cell.placeholder) {
        cell.placeholder->hide();
    }

    // Add camera widget
    cell.widget = cameraWidget;
    cell.isEmpty = false;
    cameraWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gridLayout_->addWidget(cameraWidget, row, col);

    // Map camera ID to position
    cameraIdToPosition_[cameraId] = {row, col};

    // Update state and emit signals
    updateGridState();
    emit cameraAdded(cameraId, row, col);

    return true;
}

bool CameraGridWidget::removeCamera(int cameraId) {
    // Find camera position
    auto it = cameraIdToPosition_.find(cameraId);
    if (it == cameraIdToPosition_.end()) {
        return false; // Not found
    }

    int row = it->second.first;
    int col = it->second.second;
    GridCell& cell = cells_[row][col];

    // Remove camera widget
    if (cell.widget) {
        gridLayout_->removeWidget(cell.widget);
        // Note: Don't delete the widget here - MainWindow owns it
        cell.widget = nullptr;
    }

    // Show placeholder
    if (cell.placeholder) {
        cell.placeholder->show();
    }

    cell.isEmpty = true;

    // Remove from mapping
    cameraIdToPosition_.erase(it);

    // Update state and emit signals
    updateGridState();
    emit cameraRemoved(cameraId);

    return true;
}

CameraWidget* CameraGridWidget::getCamera(int cameraId) const {
    auto it = cameraIdToPosition_.find(cameraId);
    if (it == cameraIdToPosition_.end()) {
        return nullptr;
    }

    int row = it->second.first;
    int col = it->second.second;
    return cells_[row][col].widget;
}

bool CameraGridWidget::isFull() const {
    return cameraIdToPosition_.size() >= (GRID_ROWS * GRID_COLS);
}

bool CameraGridWidget::isEmpty() const {
    return cameraIdToPosition_.empty();
}

int CameraGridWidget::getCameraCount() const {
    return static_cast<int>(cameraIdToPosition_.size());
}

void CameraGridWidget::clearAllCameras() {
    // Remove all cameras
    std::vector<int> cameraIds = getCameraIds();
    for (int cameraId : cameraIds) {
        removeCamera(cameraId);
    }
}

std::vector<int> CameraGridWidget::getCameraIds() const {
    std::vector<int> ids;
    ids.reserve(cameraIdToPosition_.size());

    for (const auto& pair : cameraIdToPosition_) {
        ids.push_back(pair.first);
    }

    return ids;
}

int CameraGridWidget::getNextAvailableCell() const {
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            if (cells_[row][col].isEmpty) {
                return row * GRID_COLS + col;
            }
        }
    }
    return -1; // No available cell
}

void CameraGridWidget::updateGridState() {
    if (isFull()) {
        emit gridFull();
    } else {
        emit gridHasSpace();
    }
}
