#pragma once
#include <QMainWindow>
#include "agentclient.h"

class QChartView;
class QLineSeries;
class QValueAxis;
class QLabel;
class QLineEdit;
class QTableView;
class QSortFilterProxyModel;
class ProcessTableModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onMetrics(const Metrics& metrics);
    void onConnectionChanged(bool connected);

private:
    QWidget* createChartPanel(const QString& title, const QColor& color, QLabel*& valueLabel,
        QLineSeries*& series, QValueAxis*& axisX);
    QWidget* createProcessPanel();
    void appendPoint(QLineSeries* series, QValueAxis* axisX, double value);

    static constexpr int kHistorySeconds = 60;

    AgentClient* m_agent = nullptr;
    int m_tick = 0;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_cpuLabel = nullptr;
    QLabel* m_ramLabel = nullptr;
    QLabel* m_processCountLabel = nullptr;

    QLineSeries* m_cpuSeries = nullptr;
    QValueAxis* m_cpuAxisX = nullptr;
    QLineSeries* m_ramSeries = nullptr;
    QValueAxis* m_ramAxisX = nullptr;

    ProcessTableModel* m_model = nullptr;
    QSortFilterProxyModel* m_proxy = nullptr;
    QTableView* m_table = nullptr;
    QLineEdit* m_filter = nullptr;
};