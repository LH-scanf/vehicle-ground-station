qt_add_executable(vehicle_state_test
    vehicle_state_test.cpp
    ../src/vehicle/VehicleState.cpp
    ../src/vehicle/VehicleState.h
)

target_include_directories(vehicle_state_test PRIVATE ../src)
target_link_libraries(vehicle_state_test PRIVATE Qt6::Test)

add_test(NAME vehicle_state_test COMMAND vehicle_state_test)
