qt_add_executable(vehicle_state_test
    vehicle_state_test.cpp
    ../src/vehicle/VehicleState.cpp
    ../src/vehicle/VehicleState.h
)

target_include_directories(vehicle_state_test PRIVATE ../src)
target_link_libraries(vehicle_state_test PRIVATE Qt6::Test)

add_test(NAME vehicle_state_test COMMAND vehicle_state_test)

qt_add_executable(config_manager_test
    config_manager_test.cpp
    ../src/config/ConfigManager.cpp
    ../src/config/ConfigManager.h
)

target_include_directories(config_manager_test PRIVATE ../src)
target_link_libraries(config_manager_test PRIVATE Qt6::Test)

add_test(NAME config_manager_test COMMAND config_manager_test)

qt_add_executable(log_manager_test
    log_manager_test.cpp
    ../src/log/LogFilterModel.cpp
    ../src/log/LogFilterModel.h
    ../src/log/LogManager.cpp
    ../src/log/LogManager.h
    ../src/log/LogWriter.cpp
    ../src/log/LogWriter.h
)

target_include_directories(log_manager_test PRIVATE ../src)
target_link_libraries(log_manager_test PRIVATE Qt6::Test)

add_test(NAME log_manager_test COMMAND log_manager_test)
