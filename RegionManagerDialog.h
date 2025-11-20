#ifndef REGIONMANAGERDIALOG_H
#define REGIONMANAGERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <vector>
#include "Region.h"

class RegionManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit RegionManagerDialog(std::vector<Region>& regions, QWidget* parent = nullptr);

    const std::vector<Region>& getRegions() const { return regions_; }

private slots:
    void onRenameClicked();
    void onDeleteClicked();
    void onDeleteAllClicked();
    void updateButtonStates();

private:
    void setupUI();
    void populateList();

    std::vector<Region>& regions_;

    QListWidget* regionListWidget_;
    QPushButton* renameButton_;
    QPushButton* deleteButton_;
    QPushButton* deleteAllButton_;
    QPushButton* closeButton_;
    QLabel* infoLabel_;
};

#endif // REGIONMANAGERDIALOG_H
