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

qt_add_executable(protocol_validator_test
    protocol_validator_test.cpp
    ../src/communication/ProtocolValidator.cpp
    ../src/communication/ProtocolValidator.h
)

target_include_directories(protocol_validator_test PRIVATE ../src)
target_link_libraries(protocol_validator_test PRIVATE Qt6::Test)

add_test(NAME protocol_validator_test COMMAND protocol_validator_test)

qt_add_executable(websocket_client_test
    websocket_client_test.cpp
    ../src/communication/ProtocolValidator.cpp
    ../src/communication/ProtocolValidator.h
    ../src/communication/WebSocketClient.cpp
    ../src/communication/WebSocketClient.h
    ../src/config/ConfigManager.cpp
    ../src/config/ConfigManager.h
    ../src/log/LogManager.cpp
    ../src/log/LogManager.h
    ../src/log/LogWriter.cpp
    ../src/log/LogWriter.h
    ../src/vehicle/VehicleState.cpp
    ../src/vehicle/VehicleState.h
)

target_include_directories(websocket_client_test PRIVATE ../src)
target_link_libraries(websocket_client_test PRIVATE Qt6::Test Qt6::WebSockets)

add_test(NAME websocket_client_test COMMAND websocket_client_test)
