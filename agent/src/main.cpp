#include <drogon/drogon.h>

#include "MetricsCollector.h"
#include "MetricsWebSocket.h"

using namespace drogon;

namespace {
    constexpr uint16_t kPort = 8090;
    constexpr double kIntervalSeconds = 1.0;
}

int main()
{
    MetricsCollector::instance().sample();

    app().registerBeginningAdvice([] {
        app().getLoop()->runEvery(kIntervalSeconds, [] {
            const std::string snapshot = MetricsCollector::instance().sample();
            MetricsWebSocket::broadcast(snapshot);
            });
        LOG_INFO << "SystemMonitorAgent run on http://127.0.0.1:" << kPort;
        });

    app().setLogLevel(trantor::Logger::kInfo)
        .addListener("127.0.0.1", kPort) // locally
        .setThreadNum(2)
        .run();

    return 0;
}