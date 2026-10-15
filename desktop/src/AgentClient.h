#pragma once
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <QWebSocket>

struct ProcessInfo {
    quint32 pid = 0;
    QString name;
    double cpu = 0.0;
    double memoryMB = 0.0;
};

struct Metrics {
    qint64 timestamp = 0;
    double cpu = 0.0;
    double memUsedGB = 0.0;
    double memTotalGB = 0.0;
    double memPercent = 0.0;
    QVector<ProcessInfo> processes;
};

// Connect through WebSocket at C++ agent and transforms
// JSON messages into Metric structs. Reconnect imediatelly.
class AgentClient : public QObject
{
    Q_OBJECT

public:
    explicit AgentClient(const QUrl& url, QObject* parent = nullptr);
    void start();

signals:
    void connectionChanged(bool connected);
    void metricsReceived(const Metrics& metrics);

private slots:
    void tryConnect();
    void onTextMessage(const QString& message);

private:
    QUrl m_url;
    QWebSocket m_socket;
    QTimer m_reconnectTimer;
};