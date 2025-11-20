#include "TelegramSettingsDialog.h"
#include "TelegramBot.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <iostream>

TelegramSettingsDialog::TelegramSettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Telegram Settings");
    setMinimumWidth(500);

    setupUI();
    loadSettings();
}

void TelegramSettingsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Title label
    QLabel* titleLabel = new QLabel("Configure Telegram Bot Settings");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    mainLayout->addSpacing(10);

    // Configuration group
    QGroupBox* configGroup = new QGroupBox("Telegram Configuration");
    QFormLayout* formLayout = new QFormLayout(configGroup);

    // Enable checkbox
    enabledCheckBox_ = new QCheckBox("Enable Telegram notifications");
    formLayout->addRow("Status:", enabledCheckBox_);

    formLayout->addRow(new QLabel("")); // Spacer

    // Bot Token
    botTokenLineEdit_ = new QLineEdit();
    botTokenLineEdit_->setPlaceholderText("Enter your Telegram Bot Token");
    botTokenLineEdit_->setEchoMode(QLineEdit::Password); // Hide token for security
    formLayout->addRow("Bot Token:", botTokenLineEdit_);

    // Show/Hide token button
    QPushButton* toggleTokenButton = new QPushButton("Show");
    toggleTokenButton->setMaximumWidth(80);
    connect(toggleTokenButton, &QPushButton::clicked, [this, toggleTokenButton]() {
        if (botTokenLineEdit_->echoMode() == QLineEdit::Password) {
            botTokenLineEdit_->setEchoMode(QLineEdit::Normal);
            toggleTokenButton->setText("Hide");
        } else {
            botTokenLineEdit_->setEchoMode(QLineEdit::Password);
            toggleTokenButton->setText("Show");
        }
    });
    formLayout->addRow("", toggleTokenButton);

    // Chat ID
    chatIdLineEdit_ = new QLineEdit();
    chatIdLineEdit_->setPlaceholderText("Enter your Chat ID");
    formLayout->addRow("Chat ID:", chatIdLineEdit_);

    formLayout->addRow(new QLabel("")); // Spacer

    // Advanced settings
    QLabel* advancedLabel = new QLabel("Advanced Settings:");
    QFont advancedFont = advancedLabel->font();
    advancedFont.setBold(true);
    advancedLabel->setFont(advancedFont);
    formLayout->addRow(advancedLabel);

    // Retry count
    retryCountSpinBox_ = new QSpinBox();
    retryCountSpinBox_->setRange(0, 5);
    retryCountSpinBox_->setValue(2);
    retryCountSpinBox_->setToolTip("Number of retry attempts if sending fails");
    formLayout->addRow("Retry Count:", retryCountSpinBox_);

    // Timeout
    timeoutSpinBox_ = new QSpinBox();
    timeoutSpinBox_->setRange(5000, 60000);
    timeoutSpinBox_->setSingleStep(1000);
    timeoutSpinBox_->setValue(10000);
    timeoutSpinBox_->setSuffix(" ms");
    timeoutSpinBox_->setToolTip("Network timeout in milliseconds");
    formLayout->addRow("Timeout:", timeoutSpinBox_);

    mainLayout->addWidget(configGroup);

    // Status label
    statusLabel_ = new QLabel("");
    statusLabel_->setWordWrap(true);
    statusLabel_->setStyleSheet("QLabel { padding: 5px; }");
    mainLayout->addWidget(statusLabel_);

    // Help text
    QLabel* helpLabel = new QLabel(
        "<b>How to get Bot Token and Chat ID:</b><br>"
        "1. Create a bot: Talk to <a href='https://t.me/BotFather'>@BotFather</a> on Telegram<br>"
        "2. Get Chat ID: Talk to <a href='https://t.me/userinfobot'>@userinfobot</a> on Telegram"
    );
    helpLabel->setOpenExternalLinks(true);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: #666; font-size: 10pt; padding: 10px; background-color: #f0f0f0; border-radius: 5px; }");
    mainLayout->addWidget(helpLabel);

    mainLayout->addStretch();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    testButton_ = new QPushButton("Test Connection");
    testButton_->setToolTip("Send a test message to verify settings");
    connect(testButton_, &QPushButton::clicked, this, &TelegramSettingsDialog::onTestConnection);
    buttonLayout->addWidget(testButton_);

    resetButton_ = new QPushButton("Reset to Default");
    connect(resetButton_, &QPushButton::clicked, this, &TelegramSettingsDialog::onResetToDefault);
    buttonLayout->addWidget(resetButton_);

    buttonLayout->addStretch();

    cancelButton_ = new QPushButton("Cancel");
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton_);

    saveButton_ = new QPushButton("Save");
    saveButton_->setDefault(true);
    connect(saveButton_, &QPushButton::clicked, this, &TelegramSettingsDialog::onSave);
    buttonLayout->addWidget(saveButton_);

    mainLayout->addLayout(buttonLayout);
}

void TelegramSettingsDialog::loadSettings() {
    QFile file(CONFIG_FILE);

    if (!file.exists()) {
        // Set default values
        enabledCheckBox_->setChecked(false);
        botTokenLineEdit_->setText("");
        chatIdLineEdit_->setText("");
        retryCountSpinBox_->setValue(2);
        timeoutSpinBox_->setValue(10000);
        showStatus("No configuration file found. Using default values.", false);
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        showStatus("Failed to open configuration file.", true);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        showStatus("Invalid JSON in configuration file.", true);
        return;
    }

    QJsonObject obj = doc.object();

    // Load values from JSON
    enabledCheckBox_->setChecked(obj.value("enabled").toBool(false));
    botTokenLineEdit_->setText(obj.value("botToken").toString());
    chatIdLineEdit_->setText(obj.value("chatId").toString());
    retryCountSpinBox_->setValue(obj.value("retryCount").toInt(2));
    timeoutSpinBox_->setValue(obj.value("timeout").toInt(10000));

    showStatus("Configuration loaded successfully.", false);
}

bool TelegramSettingsDialog::saveSettings() {
    // Validate input
    if (enabledCheckBox_->isChecked()) {
        if (botTokenLineEdit_->text().trimmed().isEmpty()) {
            showStatus("Bot Token cannot be empty when Telegram is enabled.", true);
            botTokenLineEdit_->setFocus();
            return false;
        }

        if (chatIdLineEdit_->text().trimmed().isEmpty()) {
            showStatus("Chat ID cannot be empty when Telegram is enabled.", true);
            chatIdLineEdit_->setFocus();
            return false;
        }
    }

    // Create JSON object
    QJsonObject obj;
    obj["enabled"] = enabledCheckBox_->isChecked();
    obj["botToken"] = botTokenLineEdit_->text().trimmed();
    obj["chatId"] = chatIdLineEdit_->text().trimmed();
    obj["retryCount"] = retryCountSpinBox_->value();
    obj["timeout"] = timeoutSpinBox_->value();

    // Write to file
    QFile file(CONFIG_FILE);
    if (!file.open(QIODevice::WriteOnly)) {
        showStatus("Failed to save configuration file.", true);
        return false;
    }

    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    showStatus("Configuration saved successfully!", false);

    // Reload TelegramBot configuration
    TelegramBot::getInstance().reloadConfig();

    return true;
}

void TelegramSettingsDialog::onSave() {
    if (saveSettings()) {
        QMessageBox::information(this, "Success",
            "Telegram settings saved successfully!\n\n"
            "The new configuration will be used immediately.");
        accept();
    }
}

void TelegramSettingsDialog::onTestConnection() {
    // Validate inputs first
    QString botToken = botTokenLineEdit_->text().trimmed();
    QString chatId = chatIdLineEdit_->text().trimmed();

    if (botToken.isEmpty()) {
        showStatus("Please enter Bot Token first.", true);
        return;
    }

    if (chatId.isEmpty()) {
        showStatus("Please enter Chat ID first.", true);
        return;
    }

    showStatus("Testing connection... Please wait.", false);
    testButton_->setEnabled(false);

    // Send test message via Telegram API
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    QString urlString = QString("https://api.telegram.org/bot%1/sendMessage").arg(botToken);
    QString message = "✅ Test message from YOLOv8 Multi-Camera System\n\n"
                      "If you receive this message, your Telegram configuration is correct!";

    QUrlQuery query;
    query.addQueryItem("chat_id", chatId);
    query.addQueryItem("text", message);

    QUrl url(urlString);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray postData = query.toString().toUtf8();
    QNetworkReply* reply = manager->post(request, postData);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        testButton_->setEnabled(true);

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);

            if (doc.object().value("ok").toBool()) {
                showStatus("✅ Test successful! Check your Telegram for the message.", false);
                QMessageBox::information(this, "Success",
                    "Test message sent successfully!\n\n"
                    "Check your Telegram to confirm receipt.");
            } else {
                QString error = doc.object().value("description").toString();
                showStatus(QString("❌ Test failed: %1").arg(error), true);
            }
        } else {
            showStatus(QString("❌ Network error: %1").arg(reply->errorString()), true);
        }

        reply->deleteLater();
    });
}

void TelegramSettingsDialog::onResetToDefault() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Reset to Default",
        "Are you sure you want to reset all settings to default values?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        enabledCheckBox_->setChecked(false);
        botTokenLineEdit_->setText("");
        chatIdLineEdit_->setText("");
        retryCountSpinBox_->setValue(2);
        timeoutSpinBox_->setValue(10000);
        showStatus("Settings reset to default values. Click Save to apply.", false);
    }
}

void TelegramSettingsDialog::showStatus(const QString& message, bool isError) {
    if (isError) {
        statusLabel_->setStyleSheet("QLabel { color: red; background-color: #ffe6e6; padding: 5px; border-radius: 3px; }");
    } else {
        statusLabel_->setStyleSheet("QLabel { color: green; background-color: #e6ffe6; padding: 5px; border-radius: 3px; }");
    }
    statusLabel_->setText(message);
}

// Getters
QString TelegramSettingsDialog::getBotToken() const {
    return botTokenLineEdit_->text().trimmed();
}

QString TelegramSettingsDialog::getChatId() const {
    return chatIdLineEdit_->text().trimmed();
}

bool TelegramSettingsDialog::isEnabled() const {
    return enabledCheckBox_->isChecked();
}

int TelegramSettingsDialog::getRetryCount() const {
    return retryCountSpinBox_->value();
}

int TelegramSettingsDialog::getTimeout() const {
    return timeoutSpinBox_->value();
}
