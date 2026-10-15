#pragma once
#include <drogon/HttpController.h>

//REST endpoints:
//   GET /api/health   -> verify if agent runs
//   GET /api/metrics  -> last snapshot
class MetricsController : public drogon::HttpController<MetricsController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(MetricsController::health, "/api/health", drogon::Get);
        ADD_METHOD_TO(MetricsController::metrics, "/api/metrics", drogon::Get);
    METHOD_LIST_END

    void health(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void metrics(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};