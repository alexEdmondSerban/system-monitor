#include "mainwindow.h"
#include "processtablemodel.h"

#include <QChart>
#include <QChartView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLineSeries>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QValueAxis>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("System monitor");
    resize(1000, 750);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    // Status conexiune
    m_statusLabel = new QLabel(central);
    layout->addWidget(m_statusLabel);

    // Graficele, unul lângă altul
    auto* charts = new QHBoxLayout;
    charts->addWidget(createChartPanel("CPU", QColor("#2196F3"), m_cpuLabel, m_cpuSeries, m_cpuAxisX));
    charts->addWidget(createChartPanel("RAM", QColor("#4CAF50"), m_ramLabel, m_ramSeries, m_ramAxisX));
    layout->addLayout(charts);

    // Tabelul de procese ocupă restul spațiului
    layout->addWidget(createProcessPanel(), 1);

    setCentralWidget(central);

    m_agent = new AgentClient(QUrl("ws://127.0.0.1:8090/ws/metrics"), this);
    connect(m_agent, &AgentClient::metricsReceived, this, &MainWindow::onMetrics);
    connect(m_agent, &AgentClient::connectionChanged, this, &MainWindow::onConnectionChanged);

    onConnectionChanged(false);
    m_agent->start();
}

QWidget* MainWindow::createChartPanel(const QString& title, const QColor& color, QLabel*& valueLabel,
    QLineSeries*& series, QValueAxis*& axisX)
{
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);

    valueLabel = new QLabel(title + ": -", panel);
    QFont font = valueLabel->font();
    font.setPointSize(12);
    font.setBold(true);
    valueLabel->setFont(font);
    layout->addWidget(valueLabel);

    series = new QLineSeries;
    series->setPen(QPen(color, 2));

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setMargins(QMargins(4, 4, 4, 4));

    axisX = new QValueAxis;
    axisX->setRange(0, kHistorySeconds);
    axisX->setLabelsVisible(false);

    auto* axisY = new QValueAxis;
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%d");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    auto* view = new QChartView(chart, panel);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(180);
    layout->addWidget(view);

    return panel;
}

QWidget* MainWindow::createProcessPanel()
{
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* header = new QHBoxLayout;
    m_filter = new QLineEdit(panel);
    m_filter->setPlaceholderText("Look for proccess...");
    m_filter->setClearButtonEnabled(true);
    m_processCountLabel = new QLabel(panel);
    header->addWidget(m_filter, 1);
    header->addWidget(m_processCountLabel);
    layout->addLayout(header);

    m_model = new ProcessTableModel(this);

    // Proxy-ul se ocupă de sortare și filtrare, fără să atingă modelul
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(Qt::UserRole);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(ProcessTableModel::Name);
    connect(m_filter, &QLineEdit::textChanged, m_proxy, &QSortFilterProxyModel::setFilterFixedString);

    m_table = new QTableView(panel);
    m_table->setModel(m_proxy);
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(ProcessTableModel::Cpu, Qt::DescendingOrder);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(ProcessTableModel::Name, QHeaderView::Stretch);
    layout->addWidget(m_table);

    return panel;
}

void MainWindow::appendPoint(QLineSeries* series, QValueAxis* axisX, double value)
{
    series->append(m_tick, value);
    if (series->count() > kHistorySeconds + 1)
        series->remove(0);
    if (m_tick > kHistorySeconds)
        axisX->setRange(m_tick - kHistorySeconds, m_tick);
}

void MainWindow::onMetrics(const Metrics& m)
{
    appendPoint(m_cpuSeries, m_cpuAxisX, m.cpu);
    appendPoint(m_ramSeries, m_ramAxisX, m.memPercent);
    ++m_tick;

    m_cpuLabel->setText(QString("CPU: %1%").arg(m.cpu, 0, 'f', 1));
    m_ramLabel->setText(QString("RAM: %1 / %2 GB (%3%)")
        .arg(m.memUsedGB, 0, 'f', 1)
        .arg(m.memTotalGB, 0, 'f', 1)
        .arg(m.memPercent, 0, 'f', 1));

    m_model->updateProcesses(m.processes);
    m_processCountLabel->setText(QString("%1 procese").arg(m.processes.size()));
}

void MainWindow::onConnectionChanged(bool connected)
{
    if (connected) {
        m_statusLabel->setText("● Connected at agent (127.0.0.1:8090)");
        m_statusLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
    }
    else {
        m_statusLabel->setText("● Agent not running, retry......");
        m_statusLabel->setStyleSheet("color: #c62828; font-weight: bold;");
    }
}