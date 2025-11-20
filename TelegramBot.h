#ifndef TELEGRAMBOT_H
#define TELEGRAMBOT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QString>
#include <QMutex>
#include <memory>

/**
 * @brief Singleton class for sending photos to Telegram Bot asynchronously
 *
 * This class handles sending cropped detection images to a Telegram bot
 * using Qt's QNetworkAccessManager. It runs in a separate thread to avoid
 * blocking the main YOLO processing pipeline.
 *
 * Configuration is loaded from telegram_config.json file.
 */
class TelegramBot : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     */
    static TelegramBot& getInstance();

    // Delete copy/move constructors and assignment operators
    TelegramBot(const TelegramBot&) = delete;
    TelegramBot& operator=(const TelegramBot&) = delete;
    TelegramBot(TelegramBot&&) = delete;
    TelegramBot& operator=(TelegramBot&&) = delete;

    /**
     * @brief Send a photo to Telegram with caption
     * @param pixmap The image to send (crop from detection)
     * @param caption Caption text with detection metadata
     *
     * This method is async - it returns immediately and sends the photo
     * in a background thread. Progress is logged to console.
     */
    void sendPhoto(const QPixmap& pixmap, const QString& caption);

    /**
     * @brief Check if Telegram bot is enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Reload configuration from telegram_config.json
     */
    void reloadConfig();

    /**
     * @brief Get bot token
     */
    QString getBotToken() const { return botToken_; }

    /**
     * @brief Get chat ID
     */
    QString getChatId() const { return chatId_; }

signals:
    /**
     * @brief Emitted when photo is sent successfully
     */
    void photoSent(const QString& caption);

    /**
     * @brief Emitted when sending fails
     */
    void sendFailed(const QString& caption, const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    TelegramBot();
    ~TelegramBot();

    void loadConfig();
    QByteArray pixmapToJpegBytes(const QPixmap& pixmap) const;

    // Configuration
    bool enabled_;
    QString botToken_;
    QString chatId_;
    int retryCount_;
    int timeout_;

    // Network manager
    QNetworkAccessManager* networkManager_;

    // Thread safety
    mutable QMutex mutex_;

    // Pending requests tracking (for retry logic)
    struct PendingRequest {
        QByteArray imageData;
        QString caption;
        int attemptCount;
    };
    QMap<QNetworkReply*, PendingRequest> pendingRequests_;

    static constexpr const char* CONFIG_FILE = "telegram_config.json";
};

#endif // TELEGRAMBOT_H
