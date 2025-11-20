#ifndef ADDCAMERADIALOG_H
#define ADDCAMERADIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QFileDialog>
#include "CameraSource.h"

class AddCameraDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddCameraDialog(QWidget* parent = nullptr);

    QString getCameraName() const;
    CameraType getCameraType() const;
    QString getCameraSource() const;

private slots:
    void onTypeChanged(int index);
    void onBrowseClicked();
    void onOkClicked();

private:
    void setupUI();
    bool validateInput();

    QLineEdit* nameEdit_;
    QComboBox* typeComboBox_;
    QLineEdit* sourceEdit_;
    QPushButton* browseButton_;
    QPushButton* okButton_;
    QPushButton* cancelButton_;
};

#endif // ADDCAMERADIALOG_H
