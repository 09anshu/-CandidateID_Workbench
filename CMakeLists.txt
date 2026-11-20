cmake_minimum_required(VERSION 3.16)
project(UnifiedEngineeringWorkbench VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimization flags
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3)
endif()

# Core sources (shared between backend server and tests)
set(CORE_SOURCES
    src/core/linear_algebra.cpp
    src/geometry/stl_parser.cpp
    src/geometry/csv_parser.cpp
    src/thermal/thermal_solver.cpp
    src/performance/parallel_solver.cpp
    src/hardware/mcu_adapter.cpp
    src/hardware/fpga_adapter.cpp
    src/hardware/jtag_adapter.cpp
    src/hardware/device_manager.cpp
    src/backpropagation/backprop.cpp
)

# Backend server sources
set(BACKEND_SOURCES
    ${CORE_SOURCES}
    src/thermal/heatsink_mesh.cpp
    src/thermal/heatsink_solver.cpp
    src/backend/http_server.cpp
    src/backend/server_routes.cpp
    src/backend/test_runner.cpp
    src/backend/main.cpp
)

# Custom test sources
set(TEST_SOURCES
    ${CORE_SOURCES}
    tests/test_main.cpp
)

# Backend Executable
add_executable(workbench_backend ${BACKEND_SOURCES})
target_include_directories(workbench_backend PRIVATE src src/backend/third_party)
target_compile_definitions(workbench_backend PRIVATE AMGCL_NO_BOOST)
if(WIN32)
    target_link_libraries(workbench_backend PRIVATE ws2_32 psapi)
endif()

# Custom Lightweight Test Executable
add_executable(workbench_tests ${TEST_SOURCES})
target_include_directories(workbench_tests PRIVATE src)
if(WIN32)
    target_link_libraries(workbench_tests PRIVATE ws2_32)
endif()

# GoogleTest Integration
find_package(GTest REQUIRED)
include(GoogleTest)

set(GTEST_SOURCES
    ${CORE_SOURCES}
    tests/gtest_suite.cpp
)

add_executable(workbench_gtests ${GTEST_SOURCES})
target_include_directories(workbench_gtests PRIVATE src)
target_link_libraries(workbench_gtests PRIVATE GTest::GTest GTest::Main)
if(WIN32)
    target_link_libraries(workbench_gtests PRIVATE ws2_32)
endif()

# Heatsink Thermal Benchmark Sources
set(HEATSINK_SOURCES
    src/thermal/heatsink_mesh.cpp
    src/thermal/heatsink_solver.cpp
    src/backend/heatsink_main.cpp
)

add_executable(heatsink_benchmark ${HEATSINK_SOURCES})
target_include_directories(heatsink_benchmark PRIVATE src src/backend/third_party)
target_compile_definitions(heatsink_benchmark PRIVATE AMGCL_NO_BOOST)
if(WIN32)
    target_link_libraries(heatsink_benchmark PRIVATE ws2_32 psapi)
endif()

enable_testing()
add_test(NAME ComprehensiveCustomTests COMMAND workbench_tests)
gtest_discover_tests(workbench_gtests XML_OUTPUT_DIR "${CMAKE_BINARY_DIR}/results")
