#ifndef TELEGRAMSETTINGSDIALOG_H
#define TELEGRAMSETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

/**
 * @brief Dialog for configuring Telegram bot settings
 *
 * Allows users to input and save Telegram bot configuration:
 * - Bot Token
 * - Chat ID
 * - Enable/Disable
 * - Retry count and timeout
 *
 * Settings are saved to telegram_config.json file.
 */
class TelegramSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit TelegramSettingsDialog(QWidget* parent = nullptr);

    // Getters for configuration values
    QString getBotToken() const;
    QString getChatId() const;
    bool isEnabled() const;
    int getRetryCount() const;
    int getTimeout() const;

private slots:
    void onSave();
    void onTestConnection();
    void onResetToDefault();

private:
    void setupUI();
    void loadSettings();
    bool saveSettings();
    void showStatus(const QString& message, bool isError = false);

    // UI Components
    QCheckBox* enabledCheckBox_;
    QLineEdit* botTokenLineEdit_;
    QLineEdit* chatIdLineEdit_;
    QSpinBox* retryCountSpinBox_;
    QSpinBox* timeoutSpinBox_;

    QPushButton* testButton_;
    QPushButton* saveButton_;
    QPushButton* cancelButton_;
    QPushButton* resetButton_;

    QLabel* statusLabel_;

    // Configuration file path
    static constexpr const char* CONFIG_FILE = "telegram_config.json";
};

#endif // TELEGRAMSETTINGSDIALOG_H
