#include "cameraviewwidget.h"
#include <QPainter>
#include <QFont>

CameraViewWidget::CameraViewWidget(QWidget *parent)
    : QWidget(parent)
    , m_hasCamera(false)
    , m_cameraId(-1)
    , m_isHovered(false)
{
    // Thiết lập nền đen
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    // Thiết lập size policy để tự động mở rộng
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Thiết lập minimum size để đảm bảo hiển thị tốt
    setMinimumSize(200, 150);

    // Bật mouse tracking để nhận sự kiện hover
    setMouseTracking(true);

    // Thiết lập con trỏ chuột
    setCursor(Qt::PointingHandCursor);
}

CameraViewWidget::~CameraViewWidget()
{
}

void CameraViewWidget::setHasCamera(bool hasCamera)
{
    m_hasCamera = hasCamera;
    update(); // Yêu cầu vẽ lại widget
}

void CameraViewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Vẽ nền đen
    painter.fillRect(rect(), Qt::black);

    if (!m_hasCamera)
    {
        // Vẽ viền khi hover
        if (m_isHovered)
        {
            QPen pen(QColor(80, 80, 80), 2);
            painter.setPen(pen);
            painter.drawRect(rect().adjusted(1, 1, -1, -1));
        }

        // Vẽ text "Add Camera +"
        QFont font = painter.font();
        font.setPointSize(14);
        font.setBold(false);
        painter.setFont(font);

        // Màu xám cho text
        painter.setPen(QColor(120, 120, 120));

        // Vẽ text ở giữa
        QString text = "+ Add Camera";
        painter.drawText(rect(), Qt::AlignCenter, text);

        // Vẽ icon "+" lớn hơn phía trên text
        font.setPointSize(32);
        painter.setFont(font);
        QRect iconRect = rect();
        iconRect.setBottom(rect().center().y() - 10);
        painter.drawText(iconRect, Qt::AlignCenter | Qt::AlignBottom, "+");
    }
    else
    {
        // Khi có camera, có thể hiển thị video frame ở đây
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
                        QString("Camera %1\n(Video feed here)").arg(m_cameraId));
    }
}

void CameraViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (!m_hasCamera)
        {
            emit addCameraRequested(m_cameraId);
        }
        emit clicked();
    }
    QWidget::mouseReleaseEvent(event);
}

void CameraViewWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    m_isHovered = true;
    update();
}

void CameraViewWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_isHovered = false;
    update();
}
