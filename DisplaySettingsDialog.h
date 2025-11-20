#ifndef DISPLAYSETTINGSDIALOG_H
#define DISPLAYSETTINGSDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QPushButton>
#include <QFormLayout>
#include <QSettings>

class DisplaySettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit DisplaySettingsDialog(QWidget* parent = nullptr);

    int getCameraWidth() const;
    int getCameraHeight() const;
    int getGridRows() const;
    int getGridColumns() const;

private slots:
    void onResetToDefault();
    void onSave();

private:
    void setupUI();
    void loadSettings();

    QSpinBox* widthSpinBox_;
    QSpinBox* heightSpinBox_;
    QSpinBox* rowsSpinBox_;
    QSpinBox* columnsSpinBox_;

    QPushButton* resetButton_;
    QPushButton* saveButton_;
    QPushButton* cancelButton_;

    QSettings* settings_;

    int savedWidth_;
    int savedHeight_;
    int savedRows_;
    int savedColumns_;
};

#endif // DISPLAYSETTINGSDIALOG_H
