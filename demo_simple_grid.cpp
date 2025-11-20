#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QStatusBar>
#include <QMouseEvent>
#include <QMessageBox>
#include <QVector>

/**
 * CameraCell - Widget đại diện cho một ô camera trong grid
 * Hiển thị "Add Camera +" khi chưa có video
 */
class CameraCell : public QWidget
{
    Q_OBJECT

public:
    explicit CameraCell(int id, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_cameraId(id)
        , m_hasCamera(false)
        , m_isHovered(false)
    {
        setAutoFillBackground(true);
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::black);
        setPalette(pal);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(150, 100);
        setMouseTracking(true);
        setCursor(Qt::PointingHandCursor);
    }

    void setHasCamera(bool has) {
        m_hasCamera = has;
        update();
    }

    int cameraId() const { return m_cameraId; }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Vẽ nền đen
        painter.fillRect(rect(), Qt::black);

        if (!m_hasCamera) {
            // Vẽ viền khi hover
            if (m_isHovered) {
                QPen pen(QColor(80, 80, 80), 2);
                painter.setPen(pen);
                painter.drawRect(rect().adjusted(1, 1, -1, -1));
            }

            // Vẽ icon "+" lớn
            QFont iconFont = painter.font();
            iconFont.setPointSize(48);
            iconFont.setBold(true);
            painter.setFont(iconFont);
            painter.setPen(QColor(120, 120, 120));

            QRect topHalf = rect();
            topHalf.setBottom(rect().center().y());
            painter.drawText(topHalf, Qt::AlignCenter | Qt::AlignBottom, "+");

            // Vẽ text "Add Camera"
            QFont textFont = painter.font();
            textFont.setPointSize(14);
            textFont.setBold(false);
            painter.setFont(textFont);

            QRect bottomHalf = rect();
            bottomHalf.setTop(rect().center().y() + 10);
            painter.drawText(bottomHalf, Qt::AlignCenter | Qt::AlignTop, "Add Camera");
        }
        else {
            // Khi có camera, hiển thị placeholder cho video
            painter.setPen(Qt::white);
            QFont font = painter.font();
            font.setPointSize(16);
            painter.setFont(font);
            painter.drawText(rect(), Qt::AlignCenter,
                           QString("Camera %1\n(Video stream)").arg(m_cameraId));
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && !m_hasCamera) {
            emit clicked(m_cameraId);
        }
        QWidget::mouseReleaseEvent(event);
    }

    void enterEvent(QEnterEvent *event) override {
        Q_UNUSED(event);
        m_isHovered = true;
        update();
    }

    void leaveEvent(QEvent *event) override {
        Q_UNUSED(event);
        m_isHovered = false;
        update();
    }

signals:
    void clicked(int cameraId);

private:
    int m_cameraId;
    bool m_hasCamera;
    bool m_isHovered;
};

/**
 * SimpleGridWindow - Cửa sổ demo với mainDisplay và grid layout
 */
class SimpleGridWindow : public QMainWindow
{
    Q_OBJECT

public:
    SimpleGridWindow(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , mainDisplay(nullptr)
        , gridLayout(nullptr)
        , currentRows(2)
        , currentCols(2)
    {
        setupUI();
        setupMenuBar();

        // Tạo grid mặc định 2×2
        createCameraGrid(2, 2);

        resize(1200, 800);
        setWindowTitle("YOLOv8 Multi-Camera Tracking System - Simple Grid Demo");
    }

    ~SimpleGridWindow() {}

    /**
     * createCameraGrid - Tạo grid layout với số hàng và cột chỉ định
     * @param rows: Số hàng
     * @param cols: Số cột
     */
    void createCameraGrid(int rows, int cols) {
        clearGrid();

        currentRows = rows;
        currentCols = cols;

        // Tạo layout grid mới
        gridLayout = new QGridLayout(mainDisplay);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setSpacing(0); // Không có khoảng cách giữa các ô

        // Tạo các camera cell và thêm vào grid
        int cameraId = 0;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                CameraCell *cell = new CameraCell(cameraId++, mainDisplay);

                // Kết nối signal để xử lý click
                connect(cell, &CameraCell::clicked,
                       this, &SimpleGridWindow::onCameraCellClicked);

                // Thêm vào grid
                gridLayout->addWidget(cell, row, col);
                cameraCells.append(cell);
            }
        }

        // *** QUAN TRỌNG: Thiết lập stretch để các ô chia đều ***
        // Mỗi hàng có stretch factor = 1
        for (int row = 0; row < rows; ++row) {
            gridLayout->setRowStretch(row, 1);
        }

        // Mỗi cột có stretch factor = 1
        for (int col = 0; col < cols; ++col) {
            gridLayout->setColumnStretch(col, 1);
        }

        // Cập nhật status bar
        statusBar()->showMessage(
            QString("Grid: %1×%2 (%3 cells) | Resize window to see auto-scaling")
                .arg(rows).arg(cols).arg(rows * cols)
        );
    }

private slots:
    void onCameraCellClicked(int cameraId) {
        // Khi click vào cell, giả lập thêm camera
        if (cameraId < cameraCells.size()) {
            cameraCells[cameraId]->setHasCamera(true);
            statusBar()->showMessage(
                QString("Camera %1 added! (Demo mode)").arg(cameraId), 3000
            );
        }
    }

    void changeToGrid2x2() {
        createCameraGrid(2, 2);
    }

    void changeToGrid3x3() {
        createCameraGrid(3, 3);
    }

    void changeToGrid4x4() {
        createCameraGrid(4, 4);
    }

    void showAbout() {
        QMessageBox::information(this, "About",
            "YOLOv8 Multi-Camera Tracking System\n"
            "Simple Grid Demo\n\n"
            "Tính năng:\n"
            "• mainDisplay widget chứa các camera view\n"
            "• Grid layout tự động chia đều khi resize\n"
            "• Hỗ trợ 2×2, 3×3, 4×4 layout\n"
            "• Mỗi cell hiển thị 'Add Camera +' khi chưa có video\n\n"
            "Cách sử dụng:\n"
            "1. Chọn grid layout từ menu 'Display Settings'\n"
            "2. Click vào ô để 'thêm camera' (demo)\n"
            "3. Resize cửa sổ để thấy auto-scaling"
        );
    }

private:
    void setupUI() {
        // Tạo widget trung tâm
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // Layout chính (không margin, không spacing)
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // *** mainDisplay - widget chứa tất cả camera view ***
        mainDisplay = new QWidget(centralWidget);
        mainDisplay->setAutoFillBackground(true);
        QPalette pal = mainDisplay->palette();
        pal.setColor(QPalette::Window, QColor(20, 20, 20)); // Nền tối
        mainDisplay->setPalette(pal);

        // mainDisplay tự động mở rộng để lấp đầy vùng trung tâm
        mainDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        mainLayout->addWidget(mainDisplay);

        // Status bar
        setStatusBar(new QStatusBar(this));
        statusBar()->showMessage("Ready");
    }

    void setupMenuBar() {
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);

        // Menu "Display Settings"
        QMenu *displayMenu = menuBar->addMenu("Display Settings");

        QActionGroup *layoutGroup = new QActionGroup(this);
        layoutGroup->setExclusive(true);

        // Action 2×2
        QAction *action2x2 = new QAction("2×2 Grid", this);
        action2x2->setCheckable(true);
        action2x2->setChecked(true);
        layoutGroup->addAction(action2x2);
        displayMenu->addAction(action2x2);
        connect(action2x2, &QAction::triggered, this, &SimpleGridWindow::changeToGrid2x2);

        // Action 3×3
        QAction *action3x3 = new QAction("3×3 Grid", this);
        action3x3->setCheckable(true);
        layoutGroup->addAction(action3x3);
        displayMenu->addAction(action3x3);
        connect(action3x3, &QAction::triggered, this, &SimpleGridWindow::changeToGrid3x3);

        // Action 4×4
        QAction *action4x4 = new QAction("4×4 Grid", this);
        action4x4->setCheckable(true);
        layoutGroup->addAction(action4x4);
        displayMenu->addAction(action4x4);
        connect(action4x4, &QAction::triggered, this, &SimpleGridWindow::changeToGrid4x4);

        // Menu "Help"
        QMenu *helpMenu = menuBar->addMenu("Help");
        QAction *aboutAction = new QAction("About", this);
        helpMenu->addAction(aboutAction);
        connect(aboutAction, &QAction::triggered, this, &SimpleGridWindow::showAbout);
    }

    void clearGrid() {
        // Xóa tất cả camera cells
        for (CameraCell *cell : cameraCells) {
            cell->deleteLater();
        }
        cameraCells.clear();

        // Xóa layout cũ
        if (gridLayout) {
            QLayoutItem *item;
            while ((item = gridLayout->takeAt(0)) != nullptr) {
                delete item;
            }
            delete gridLayout;
            gridLayout = nullptr;
        }
    }

    // Widget chính chứa các camera view
    QWidget *mainDisplay;

    // Layout grid
    QGridLayout *gridLayout;

    // Danh sách các camera cells
    QVector<CameraCell*> cameraCells;

    // Số hàng và cột hiện tại
    int currentRows;
    int currentCols;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Thiết lập style tối
    app.setStyle("Fusion");

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    app.setPalette(darkPalette);

    SimpleGridWindow window;
    window.show();

    return app.exec();
}

#include "demo_simple_grid.moc"
