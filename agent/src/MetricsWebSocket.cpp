#include "MetricsWebSocket.h"
#include "MetricsCollector.h"

#include <vector>

using namespace drogon;

std::mutex MetricsWebSocket::s_mutex;
std::unordered_set<WebSocketConnectionPtr> MetricsWebSocket::s_connections;

void MetricsWebSocket::handleNewConnection(const HttpRequestPtr&, const WebSocketConnectionPtr& conn)
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_connections.insert(conn);
    }
    LOG_INFO << "Client WebSocket connected (" << s_connections.size() << " total)";

    // Client gets data right away
    conn->send(MetricsCollector::instance().latest());
}

void MetricsWebSocket::handleConnectionClosed(const WebSocketConnectionPtr& conn)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_connections.erase(conn);
    LOG_INFO << "Client WebSocket deconnected (" << s_connections.size() << " left)";
}

void MetricsWebSocket::handleNewMessage(const WebSocketConnectionPtr&, std::string&&, const WebSocketMessageType&)
{
    //later
}

void MetricsWebSocket::broadcast(const std::string& message)
{
    std::vector<WebSocketConnectionPtr> targets;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        targets.assign(s_connections.begin(), s_connections.end());
    }
    for (const auto& conn : targets) {
        if (conn->connected())
            conn->send(message);
    }
}