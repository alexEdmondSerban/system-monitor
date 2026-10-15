#pragma once
#include <drogon/WebSocketController.h>
#include <mutex>
#include <unordered_set>

// WebSocket on ws://127.0.0.1:8090/ws/metrics
//Every connected client gets a new snapshot every second

class MetricsWebSocket : public drogon::WebSocketController<MetricsWebSocket>
{
public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr& conn, std::string&& message, const drogon::WebSocketMessageType& type) override;
    void handleNewConnection(const drogon::HttpRequestPtr& req, const drogon::WebSocketConnectionPtr& conn) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) override;

    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/ws/metrics");
    WS_PATH_LIST_END

    // Send message to all connected clients.
    static void broadcast(const std::string& message);

private:
    static std::mutex s_mutex;
    static std::unordered_set<drogon::WebSocketConnectionPtr> s_connections;
};