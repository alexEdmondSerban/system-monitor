#include "MetricsController.h"
#include "MetricsCollector.h"

using namespace drogon;

void MetricsController::health(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& callback)
{
    Json::Value body;
    body["status"] = "ok";
    body["service"] = "SystemMonitorAgent";
    callback(HttpResponse::newHttpJsonResponse(body));
}

void MetricsController::metrics(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& callback)
{
    auto response = HttpResponse::newHttpResponse();
    response->setContentTypeCode(CT_APPLICATION_JSON);
    response->setBody(MetricsCollector::instance().latest());
    callback(response);
}