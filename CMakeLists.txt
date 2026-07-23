cmake_minimum_required(VERSION 3.21)

project(vehicle_ground_station VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Quick QuickControls2 Test)
qt_standard_project_setup(REQUIRES 6.6)
qt_policy(SET QTP0004 NEW)

qt_add_executable(vehicle_ground_station
    src/main.cpp
    src/config/ConfigManager.cpp
    src/config/ConfigManager.h
    src/vehicle/VehicleState.cpp
    src/vehicle/VehicleState.h
)

qt_add_resources(vehicle_ground_station "configuration"
    PREFIX "/"
    FILES
        config/default_config.json
)

qt_add_qml_module(vehicle_ground_station
    URI VehicleGroundStation
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/ConnectionIndicator.qml
        qml/components/StatusCard.qml
        qml/pages/DashboardPage.qml
        qml/pages/DiagnosticsPage.qml
        qml/pages/MapPage.qml
        qml/pages/SettingsPage.qml
)

target_include_directories(vehicle_ground_station PRIVATE src)
target_link_libraries(vehicle_ground_station PRIVATE Qt6::Quick Qt6::QuickControls2)

set_target_properties(vehicle_ground_station PROPERTIES
    WIN32_EXECUTABLE TRUE
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
