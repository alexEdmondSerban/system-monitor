#include "agentclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

AgentClient::AgentClient(const QUrl& url, QObject* parent)
    : QObject(parent)
    , m_url(url)
{
    connect(&m_socket, &QWebSocket::connected, this, [this] { emit connectionChanged(true); });
    connect(&m_socket, &QWebSocket::disconnected, this, [this] { emit connectionChanged(false); });
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &AgentClient::onTextMessage);

    // check connection - 2 seconds
    m_reconnectTimer.setInterval(2000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &AgentClient::tryConnect);
}

void AgentClient::start()
{
    tryConnect();
    m_reconnectTimer.start();
}

void AgentClient::tryConnect()
{
    if (m_socket.state() == QAbstractSocket::UnconnectedState)
        m_socket.open(m_url);
}

void AgentClient::onTextMessage(const QString& message)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return;

    const QJsonObject root = doc.object();

    Metrics m;
    m.timestamp = root["timestamp"].toInteger();
    m.cpu = root["cpu"].toDouble();

    const QJsonObject memory = root["memory"].toObject();
    m.memUsedGB = memory["usedGB"].toDouble();
    m.memTotalGB = memory["totalGB"].toDouble();
    m.memPercent = memory["percent"].toDouble();

    const QJsonArray processes = root["processes"].toArray();
    m.processes.reserve(processes.size());
    for (const QJsonValue& value : processes) {
        const QJsonObject o = value.toObject();
        ProcessInfo p;
        p.pid = quint32(o["pid"].toInteger());
        p.name = o["name"].toString();
        p.cpu = o["cpu"].toDouble();
        p.memoryMB = o["memoryMB"].toDouble();
        m.processes.append(p);
    }

    emit metricsReceived(m);
}