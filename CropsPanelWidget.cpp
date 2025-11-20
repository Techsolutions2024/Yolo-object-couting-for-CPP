#include "CropsPanelWidget.h"
#include <QDateTime>
#include <QScrollBar>
#include <QTimer>
#include <algorithm>

// Static style definitions
const QString CropsPanelWidget::PANEL_STYLE =
    "QWidget { background-color: #2b2b2b; }";

const QString CropsPanelWidget::TITLE_STYLE =
    "QLabel { "
    "   color: #ffffff; "
    "   font-size: 14px; "
    "   font-weight: bold; "
    "   padding: 8px; "
    "   background-color: #1e1e1e; "
    "}";

const QString CropsPanelWidget::CLEAR_BUTTON_STYLE =
    "QPushButton { "
    "   background-color: #d9534f; "
    "   color: white; "
    "   border: none; "
    "   padding: 6px 12px; "
    "   font-size: 12px; "
    "   border-radius: 3px; "
    "} "
    "QPushButton:hover { "
    "   background-color: #c9302c; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #ac2925; "
    "}";

// ========== CropItemWidget Implementation ==========

const QString CropItemWidget::CROP_ITEM_STYLE =
    "QFrame { "
    "   background-color: #3a3a3a; "
    "   border: 1px solid #555555; "
    "   border-radius: 4px; "
    "   padding: 8px; "
    "   margin: 4px; "
    "} "
    "QLabel { "
    "   color: #e0e0e0; "
    "   font-size: 11px; "
    "}";

CropItemWidget::CropItemWidget(const CropItem& item, QWidget* parent)
    : QFrame(parent), imageLabel_(nullptr), infoLabel_(nullptr) {
    setupUI(item);
}

void CropItemWidget::setupUI(const CropItem& item) {
    setFrameShape(QFrame::StyledPanel);
    setStyleSheet(CropItemWidget::CROP_ITEM_STYLE);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Crop image
    imageLabel_ = new QLabel(this);
    imageLabel_->setAlignment(Qt::AlignCenter);

    // Scale image to fit panel width while maintaining aspect ratio
    QPixmap scaledPixmap = item.cropImage.scaled(
        200, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel_->setPixmap(scaledPixmap);
    imageLabel_->setMinimumHeight(80);
    imageLabel_->setMaximumHeight(150);

    layout->addWidget(imageLabel_);

    // Info text
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QString infoText = QString(
        "<b>Camera:</b> %1<br>"
        "<b>Class:</b> %2<br>"
        "<b>Track ID:</b> %3<br>"
        "<b>Confidence:</b> %4%<br>"
        "<b>Time:</b> %5"
    ).arg(item.cameraId)
     .arg(item.className)
     .arg(item.trackId)
     .arg(QString::number(item.confidence * 100.0f, 'f', 1))
     .arg(item.timestamp.toString("hh:mm:ss"));

    infoLabel_->setText(infoText);
    layout->addWidget(infoLabel_);

    setLayout(layout);
}

// ========== CropsPanelWidget Implementation ==========

CropsPanelWidget::CropsPanelWidget(QWidget* parent)
    : QWidget(parent),
      scrollArea_(nullptr),
      contentWidget_(nullptr),
      contentLayout_(nullptr),
      clearButton_(nullptr),
      titleLabel_(nullptr),
      countLabel_(nullptr),
      maxCrops_(100) {
    setupUI();
}

CropsPanelWidget::~CropsPanelWidget() {
    clearAllCrops();
}

void CropsPanelWidget::setupUI() {
    setStyleSheet(PANEL_STYLE);
    setMinimumWidth(250);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header section
    QWidget* headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("QWidget { background-color: #1e1e1e; }");
    QVBoxLayout* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(8, 8, 8, 8);
    headerLayout->setSpacing(4);

    // Title
    titleLabel_ = new QLabel("Realtime Detections", this);
    titleLabel_->setStyleSheet(TITLE_STYLE);
    titleLabel_->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(titleLabel_);

    // Count label
    countLabel_ = new QLabel("Count: 0", this);
    countLabel_->setStyleSheet("QLabel { color: #aaaaaa; font-size: 11px; padding: 2px 8px; }");
    countLabel_->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(countLabel_);

    // Clear button
    clearButton_ = new QPushButton("Clear All", this);
    clearButton_->setStyleSheet(CLEAR_BUTTON_STYLE);
    clearButton_->setCursor(Qt::PointingHandCursor);
    connect(clearButton_, &QPushButton::clicked, this, &CropsPanelWidget::onClearButtonClicked);
    headerLayout->addWidget(clearButton_);

    mainLayout->addWidget(headerWidget);

    // Scroll area for crops
    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea_->setStyleSheet(
        "QScrollArea { "
        "   border: none; "
        "   background-color: #2b2b2b; "
        "}"
        "QScrollBar:vertical { "
        "   background: #2b2b2b; "
        "   width: 10px; "
        "   margin: 0px; "
        "}"
        "QScrollBar::handle:vertical { "
        "   background: #555555; "
        "   border-radius: 5px; "
        "   min-height: 20px; "
        "}"
        "QScrollBar::handle:vertical:hover { "
        "   background: #666666; "
        "}"
    );

    // Content widget inside scroll area
    contentWidget_ = new QWidget();
    contentWidget_->setStyleSheet("QWidget { background-color: #2b2b2b; }");

    contentLayout_ = new QVBoxLayout(contentWidget_);
    contentLayout_->setContentsMargins(4, 4, 4, 4);
    contentLayout_->setSpacing(6);
    contentLayout_->addStretch(); // Push items to top

    contentWidget_->setLayout(contentLayout_);
    scrollArea_->setWidget(contentWidget_);

    mainLayout->addWidget(scrollArea_);
    setLayout(mainLayout);
}

void CropsPanelWidget::addCrop(const QPixmap& cropImage, const QPixmap& fullFrameImage,
                               const QString& cameraId, const QString& className,
                               int trackId, float confidence) {
    // Create crop item
    QDateTime now = QDateTime::currentDateTime();
    CropItem item(cropImage, cameraId, className, now, trackId, confidence);

    // Create widget
    CropItemWidget* cropWidget = new CropItemWidget(item, contentWidget_);

    // Insert at the beginning (most recent at top)
    contentLayout_->insertWidget(0, cropWidget);
    cropWidgets_.push_front(cropWidget);

    // Remove oldest if exceeding max
    if (cropWidgets_.size() > static_cast<size_t>(maxCrops_)) {
        CropItemWidget* oldest = cropWidgets_.back();
        cropWidgets_.pop_back();
        removeCropWidget(oldest);
    }

    // Update count
    countLabel_->setText(QString("Count: %1").arg(cropWidgets_.size()));

    // Auto-scroll to top to show newest
    QScrollBar* scrollBar = scrollArea_->verticalScrollBar();
    scrollBar->setValue(scrollBar->minimum());

    // Emit signal
    emit cropAdded(cameraId, className);

    // Send to Telegram if enabled
    if (TelegramBot::getInstance().isEnabled()) {
        // Build caption for crop image
        QString cropCaption = QString("[CROP] Camera: %1 | Class: %2 | ID: %3 | Conf: %4%")
            .arg(cameraId)
            .arg(className)
            .arg(trackId)
            .arg(QString::number(confidence * 100.0f, 'f', 1));

        // Build caption for full frame image
        QString fullCaption = QString("[FULL FRAME] Camera: %1 | Class: %2 | ID: %3 | Time: %4")
            .arg(cameraId)
            .arg(className)
            .arg(trackId)
            .arg(now.toString("hh:mm:ss"));

        // Send crop image first
        TelegramBot::getInstance().sendPhoto(cropImage, cropCaption);

        // Send full frame image after a short delay (100ms) to avoid rate limiting
        QTimer::singleShot(100, [fullFrameImage, fullCaption]() {
            TelegramBot::getInstance().sendPhoto(fullFrameImage, fullCaption);
        });
    }
}

void CropsPanelWidget::clearAllCrops() {
    // Remove all crop widgets
    for (CropItemWidget* widget : cropWidgets_) {
        contentLayout_->removeWidget(widget);
        widget->deleteLater();
    }
    cropWidgets_.clear();

    // Update count
    countLabel_->setText("Count: 0");

    // Emit signal
    emit cropsCleared();
}

void CropsPanelWidget::setMaxCrops(int maxCrops) {
    maxCrops_ = std::max(1, maxCrops); // Ensure at least 1

    // Remove excess if current count exceeds new max
    while (cropWidgets_.size() > static_cast<size_t>(maxCrops_)) {
        CropItemWidget* oldest = cropWidgets_.back();
        cropWidgets_.pop_back();
        removeCropWidget(oldest);
    }

    countLabel_->setText(QString("Count: %1").arg(cropWidgets_.size()));
}

int CropsPanelWidget::getCropCount() const {
    return static_cast<int>(cropWidgets_.size());
}

void CropsPanelWidget::onClearButtonClicked() {
    clearAllCrops();
}

void CropsPanelWidget::removeCropWidget(CropItemWidget* widget) {
    if (widget) {
        contentLayout_->removeWidget(widget);
        widget->deleteLater();
    }
}
