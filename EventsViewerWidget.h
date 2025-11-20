#ifndef EVENTSVIEWERWIDGET_H
#define EVENTSVIEWERWIDGET_H

#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <vector>
#include <algorithm>
#include "DetectionEvent.h"
#include "EventThumbnailWidget.h"
#include "EventManager.h"
#include "EventsRegionCountWidget.h"

class EventsViewerWidget : public QDialog {
    Q_OBJECT

public:
    explicit EventsViewerWidget(QWidget* parent = nullptr);

private slots:
    void onRefresh();
    void onApplyFilter();
    void onClearFilter();
    void onClearAllEvents();
    void onThumbnailClicked(const DetectionEvent& event);
    void onPreviousPage();  // NEW
    void onNextPage();      // NEW
    void onPageChanged();   // NEW

private:
    void setupUI();
    void setupEventsTab(QWidget* eventsTab);  // NEW: Setup events tab content
    void loadEvents();
    void clearThumbnails();
    void displayEvents(const std::vector<DetectionEvent>& events);
    void displayCurrentPage();    // NEW: Display events for current page
    void updatePaginationControls();  // NEW: Enable/disable pagination buttons
    int calculateOptimalColumns() const;  // NEW: Dynamic column calculation

    // NEW: Tab widget
    QTabWidget* tabWidget_;
    EventsRegionCountWidget* regionCountWidget_;

    QComboBox* cameraFilterCombo_;
    QDateTimeEdit* startDateEdit_;
    QDateTimeEdit* endDateEdit_;
    QPushButton* applyFilterButton_;
    QPushButton* clearFilterButton_;
    QPushButton* refreshButton_;
    QPushButton* clearAllButton_;
    QPushButton* closeButton_;

    // NEW: Pagination controls
    QPushButton* prevPageButton_;
    QPushButton* nextPageButton_;
    QLabel* pageInfoLabel_;
    QSpinBox* pageSpinBox_;

    QScrollArea* scrollArea_;
    QWidget* gridContainer_;
    QGridLayout* gridLayout_;
    QLabel* statusLabel_;

    std::vector<EventThumbnailWidget*> thumbnailWidgets_;
    bool filterActive_;

    // NEW: Pagination data
    static constexpr int EVENTS_PER_PAGE = 50;
    std::vector<DetectionEvent> allEvents_;  // All loaded events
    int currentPage_;
    int totalPages_;
    int currentColumns_;  // Track current column count for resize detection
};

#endif // EVENTSVIEWERWIDGET_H
