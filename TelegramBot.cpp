#include "TelegramBot.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QBuffer>
#include <QDateTime>
#include <QUrlQuery>
#include <iostream>

TelegramBot::TelegramBot()
    : QObject(nullptr),
      enabled_(false),
      retryCount_(2),
      timeout_(10000),
      networkManager_(nullptr) {

    // Create network manager
    networkManager_ = new QNetworkAccessManager(this);
    connect(networkManager_, &QNetworkAccessManager::finished,
            this, &TelegramBot::onReplyFinished);

    // Load configuration
    loadConfig();
}

TelegramBot::~TelegramBot() {
    if (networkManager_) {
        networkManager_->deleteLater();
    }
}

TelegramBot& TelegramBot::getInstance() {
    static TelegramBot instance;
    return instance;
}

void TelegramBot::loadConfig() {
    QFile file(CONFIG_FILE);

    if (!file.exists()) {
        std::cerr << "⚠️  Telegram: Config file not found: " << CONFIG_FILE << std::endl;
        std::cerr << "   Create telegram_config.json with botToken and chatId" << std::endl;
        enabled_ = false;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "❌ Telegram: Cannot open config file: " << CONFIG_FILE << std::endl;
        enabled_ = false;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        std::cerr << "❌ Telegram: Invalid JSON in config file" << std::endl;
        enabled_ = false;
        return;
    }

    QJsonObject obj = doc.object();

    enabled_ = obj.value("enabled").toBool(false);
    botToken_ = obj.value("botToken").toString();
    chatId_ = obj.value("chatId").toString();
    retryCount_ = obj.value("retryCount").toInt(2);
    timeout_ = obj.value("timeout").toInt(10000);

    if (enabled_ && (botToken_.isEmpty() || chatId_.isEmpty())) {
        std::cerr << "❌ Telegram: botToken or chatId is empty in config" << std::endl;
        enabled_ = false;
        return;
    }

    if (enabled_) {
        std::cout << "✅ Telegram: Enabled (chatId: " << chatId_.toStdString() << ")" << std::endl;
    } else {
        std::cout << "⚠️  Telegram: Disabled in config" << std::endl;
    }
}

void TelegramBot::reloadConfig() {
    QMutexLocker locker(&mutex_);
    loadConfig();
}

QByteArray TelegramBot::pixmapToJpegBytes(const QPixmap& pixmap) const {
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "JPEG", 85); // 85% quality
    buffer.close();
    return bytes;
}

void TelegramBot::sendPhoto(const QPixmap& pixmap, const QString& caption) {
    if (!enabled_) {
        return; // Silently skip if disabled
    }

    if (pixmap.isNull()) {
        std::cerr << "❌ Telegram: Cannot send null pixmap" << std::endl;
        return;
    }

    QMutexLocker locker(&mutex_);

    // Convert pixmap to JPEG bytes
    QByteArray imageData = pixmapToJpegBytes(pixmap);

    // Build multipart form data
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Add chat_id
    QHttpPart chatIdPart;
    chatIdPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"chat_id\""));
    chatIdPart.setBody(chatId_.toUtf8());
    multiPart->append(chatIdPart);

    // Add photo
    QHttpPart photoPart;
    photoPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    photoPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"photo\"; filename=\"crop.jpg\""));
    photoPart.setBody(imageData);
    multiPart->append(photoPart);

    // Add caption
    if (!caption.isEmpty()) {
        QHttpPart captionPart;
        captionPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                            QVariant("form-data; name=\"caption\""));
        captionPart.setBody(caption.toUtf8());
        multiPart->append(captionPart);
    }

    // Build URL
    QString url = QString("https://api.telegram.org/bot%1/sendPhoto").arg(botToken_);

    // Create request
    QNetworkRequest request(url);

    // Send POST request
    QNetworkReply* reply = networkManager_->post(request, multiPart);
    multiPart->setParent(reply); // Auto-delete multiPart with reply

    // Track pending request for retry logic
    PendingRequest pending;
    pending.imageData = imageData;
    pending.caption = caption;
    pending.attemptCount = 1;
    pendingRequests_[reply] = pending;

    std::cout << "📤 Telegram: Sending photo (" << caption.toStdString() << ")..." << std::endl;
}

void TelegramBot::onReplyFinished(QNetworkReply* reply) {
    QMutexLocker locker(&mutex_);

    if (!pendingRequests_.contains(reply)) {
        reply->deleteLater();
        return;
    }

    PendingRequest pending = pendingRequests_[reply];
    pendingRequests_.remove(reply);

    if (reply->error() == QNetworkReply::NoError) {
        // Success
        QByteArray response = reply->readAll();
        std::cout << "✅ Telegram: Photo sent successfully (" << pending.caption.toStdString() << ")" << std::endl;

        emit photoSent(pending.caption);

    } else {
        // Error
        QString errorString = reply->errorString();
        std::cerr << "❌ Telegram: Failed to send (" << pending.caption.toStdString() << ") - "
                  << errorString.toStdString() << std::endl;

        // Retry logic
        if (pending.attemptCount < retryCount_) {
            std::cout << "🔄 Telegram: Retrying... (attempt " << (pending.attemptCount + 1)
                      << "/" << retryCount_ << ")" << std::endl;

            // Rebuild multipart
            QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

            QHttpPart chatIdPart;
            chatIdPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                QVariant("form-data; name=\"chat_id\""));
            chatIdPart.setBody(chatId_.toUtf8());
            multiPart->append(chatIdPart);

            QHttpPart photoPart;
            photoPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
            photoPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                               QVariant("form-data; name=\"photo\"; filename=\"crop.jpg\""));
            photoPart.setBody(pending.imageData);
            multiPart->append(photoPart);

            if (!pending.caption.isEmpty()) {
                QHttpPart captionPart;
                captionPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                                    QVariant("form-data; name=\"caption\""));
                captionPart.setBody(pending.caption.toUtf8());
                multiPart->append(captionPart);
            }

            QString url = QString("https://api.telegram.org/bot%1/sendPhoto").arg(botToken_);
            QNetworkRequest request(url);

            QNetworkReply* retryReply = networkManager_->post(request, multiPart);
            multiPart->setParent(retryReply);

            // Track retry
            pending.attemptCount++;
            pendingRequests_[retryReply] = pending;

        } else {
            std::cerr << "❌ Telegram: Max retries reached, giving up (" << pending.caption.toStdString() << ")" << std::endl;
            emit sendFailed(pending.caption, errorString);
        }
    }

    reply->deleteLater();
}
