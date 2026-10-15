cmake_minimum_required(VERSION 3.21)
project(SystemMonitorDesktop VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts WebSockets)
qt_standard_project_setup()

qt_add_executable(SystemMonitor WIN32
    src/main.cpp
    src/MainWindow.h          src/MainWindow.cpp
    src/AgentClient.h         src/AgentClient.cpp
    src/ProcessTableModel.h   src/ProcessTableModel.cpp
)

target_link_libraries(SystemMonitor PRIVATE Qt6::Widgets Qt6::Charts Qt6::WebSockets)

if(MSVC)
    target_compile_options(SystemMonitor PRIVATE /utf-8)
endif()

# Copiază DLL-urile Qt lângă .exe după fiecare build.
if(WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET SystemMonitor POST_BUILD
            COMMAND "${WINDEPLOYQT}" --no-translations --no-compiler-runtime
                    "$<TARGET_FILE:SystemMonitor>"
            COMMENT "Rulez windeployqt...")
    endif()
endif()