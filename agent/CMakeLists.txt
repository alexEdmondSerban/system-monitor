cmake_minimum_required(VERSION 3.21)
project(SystemMonitorAgent VERSION 0.1 LANGUAGES CXX)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
find_package(Drogon CONFIG REQUIRED)
 
add_executable(SystemMonitorAgent
    src/main.cpp
    src/MetricsCollector.h      src/MetricsCollector.cpp
    src/MetricsController.h     src/MetricsController.cpp
    src/MetricsWebSocket.h      src/MetricsWebSocket.cpp
)
 
target_link_libraries(SystemMonitorAgent PRIVATE Drogon::Drogon psapi)
 
if(MSVC)
    target_compile_options(SystemMonitorAgent PRIVATE /utf-8)
    target_compile_definitions(SystemMonitorAgent PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
 