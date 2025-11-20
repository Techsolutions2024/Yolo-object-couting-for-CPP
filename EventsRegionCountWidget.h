#ifndef EVENTSREGIONCOUNTWIDGET_H
#define EVENTSREGIONCOUNTWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <map>
#include <set>
#include <string>

/**
 * @brief Widget to display real-time region counting statistics
 *
 * This widget shows the number of unique objects (by tracking ID) that have
 * entered each region. It automatically refreshes data from RegionCountManager
 * and provides export/clear functionality.
 */
class EventsRegionCountWidget : public QWidget {
    Q_OBJECT

public:
    explicit EventsRegionCountWidget(QWidget* parent = nullptr);
    ~EventsRegionCountWidget();

private slots:
    void onRefresh();
    void onExport();
    void onClearAll();
    void onClearRegion();
    void onAutoRefreshToggled(bool enabled);
    void updateDisplay();

private:
    void setupUI();
    void loadRegionData();
    void populateTable(const std::map<std::string, std::pair<int, std::set<size_t>>>& data);
    QString formatIdsList(const std::set<size_t>& ids) const;

    // UI components
    QTableWidget* tableWidget_;
    QPushButton* refreshButton_;
    QPushButton* exportButton_;
    QPushButton* clearAllButton_;
    QPushButton* clearRegionButton_;
    QLabel* statusLabel_;
    QLabel* updateTimeLabel_;
    QPushButton* autoRefreshToggle_;

    // Auto-refresh timer
    QTimer* refreshTimer_;
    bool autoRefreshEnabled_;

    // Constants
    static constexpr int UPDATE_INTERVAL_MS = 500;  // 500ms refresh interval
    static constexpr int MAX_IDS_DISPLAY = 20;      // Max IDs to show in table
};

#endif // EVENTSREGIONCOUNTWIDGET_H
