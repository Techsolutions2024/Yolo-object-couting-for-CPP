#ifndef CROPSPANELWIDGET_H
#define CROPSPANELWIDGET_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QDateTime>
#include <QFrame>
#include <vector>
#include <deque>
#include "TelegramBot.h"

/**
 * @brief Represents a single cropped detection item
 */
struct CropItem {
    QPixmap cropImage;
    QString cameraId;
    QString className;
    QDateTime timestamp;
    int trackId;
    float confidence;

    CropItem(const QPixmap& image, const QString& camId, const QString& cls,
             const QDateTime& time, int tid, float conf)
        : cropImage(image), cameraId(camId), className(cls),
          timestamp(time), trackId(tid), confidence(conf) {}
};

/**
 * @brief Widget to display a single crop item with metadata
 */
class CropItemWidget : public QFrame {
    Q_OBJECT

public:
    explicit CropItemWidget(const CropItem& item, QWidget* parent = nullptr);

    // Style constant
    static const QString CROP_ITEM_STYLE;

private:
    void setupUI(const CropItem& item);

    QLabel* imageLabel_;
    QLabel* infoLabel_;
};

/**
 * @brief Panel widget that displays realtime cropped detections from cameras
 *
 * This widget shows a scrollable list of cropped object images detected
 * from the camera grid, with metadata including camera ID, class name,
 * track ID, confidence, and timestamp.
 */
class CropsPanelWidget : public QWidget {
    Q_OBJECT

public:
    explicit CropsPanelWidget(QWidget* parent = nullptr);
    ~CropsPanelWidget() override;

    /**
     * @brief Add a new crop to the panel
     * @param cropImage The cropped image of the detected object
     * @param fullFrameImage The full frame image with all detections drawn
     * @param cameraId The ID/name of the source camera
     * @param className The detected object class name
     * @param trackId The tracking ID of the object
     * @param confidence The detection confidence score
     */
    void addCrop(const QPixmap& cropImage, const QPixmap& fullFrameImage,
                 const QString& cameraId, const QString& className,
                 int trackId, float confidence);

    /**
     * @brief Clear all crops from the panel
     */
    void clearAllCrops();

    /**
     * @brief Set the maximum number of crops to display (oldest will be removed)
     * @param maxCrops Maximum number of crops (default: 100)
     */
    void setMaxCrops(int maxCrops);

    /**
     * @brief Get the current number of crops displayed
     */
    int getCropCount() const;

signals:
    /**
     * @brief Emitted when a crop is added
     */
    void cropAdded(const QString& cameraId, const QString& className);

    /**
     * @brief Emitted when crops are cleared
     */
    void cropsCleared();

private slots:
    void onClearButtonClicked();

private:
    void setupUI();
    void removeCropWidget(CropItemWidget* widget);

    // UI Components
    QScrollArea* scrollArea_;
    QWidget* contentWidget_;
    QVBoxLayout* contentLayout_;
    QPushButton* clearButton_;
    QLabel* titleLabel_;
    QLabel* countLabel_;

    // Data
    std::deque<CropItemWidget*> cropWidgets_;
    int maxCrops_;

    // Styling
    static const QString PANEL_STYLE;
    static const QString TITLE_STYLE;
    static const QString CLEAR_BUTTON_STYLE;
};

#endif // CROPSPANELWIDGET_H
