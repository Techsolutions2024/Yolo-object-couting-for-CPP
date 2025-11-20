#ifndef CLASSSELECTIONDIALOG_H
#define CLASSSELECTIONDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QHeaderView>
#include <set>
#include <vector>
#include <string>

/**
 * @brief Dialog for selecting model classes to count
 *
 * Displays all classes from the YOLO model in a table with checkboxes.
 * Users can select which classes to count, with search functionality.
 * Default behavior: count all classes if none selected.
 */
class ClassSelectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ClassSelectionDialog(const std::vector<std::string>& allClasses,
                                 const std::set<int>& currentSelection,
                                 QWidget* parent = nullptr);

    // Get selected class IDs
    std::set<int> getSelectedClasses() const;

    // Check if "count all" mode is active
    bool isCountAllMode() const { return countAllMode_; }

private slots:
    void onSearchTextChanged(const QString& text);
    void onSelectAll();
    void onDeselectAll();
    void onApply();
    void onHeaderCheckboxToggled(bool checked);

private:
    void setupUI();
    void populateTable();
    void filterTable(const QString& searchText);
    void updateStatusLabel();

    std::vector<std::string> allClasses_;
    std::set<int> selectedClasses_;
    bool countAllMode_;

    QTableWidget* tableWidget_;
    QLineEdit* searchLineEdit_;
    QPushButton* selectAllButton_;
    QPushButton* deselectAllButton_;
    QPushButton* applyButton_;
    QPushButton* cancelButton_;
    QCheckBox* headerCheckBox_;
    QLabel* statusLabel_;

    static constexpr int COL_ID = 0;
    static constexpr int COL_NAME = 1;
    static constexpr int COL_CHECKBOX = 2;
};

#endif // CLASSSELECTIONDIALOG_H
