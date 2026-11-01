cmake_minimum_required(VERSION 3.16)
project(UnifiedEngineeringWorkbenchBackend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3)
endif()

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

set(CORE_SOURCES
    ${ROOT_DIR}/src/core/linear_algebra.cpp
    ${ROOT_DIR}/src/geometry/stl_parser.cpp
    ${ROOT_DIR}/src/geometry/csv_parser.cpp
    ${ROOT_DIR}/src/thermal/thermal_solver.cpp
    ${ROOT_DIR}/src/performance/parallel_solver.cpp
    ${ROOT_DIR}/src/hardware/mcu_adapter.cpp
    ${ROOT_DIR}/src/hardware/fpga_adapter.cpp
    ${ROOT_DIR}/src/hardware/jtag_adapter.cpp
    ${ROOT_DIR}/src/hardware/device_manager.cpp
    ${ROOT_DIR}/src/backpropagation/backprop.cpp
)

set(BACKEND_SOURCES
    ${CORE_SOURCES}
    ${ROOT_DIR}/src/backend/http_server.cpp
    ${ROOT_DIR}/src/backend/server_routes.cpp
    ${ROOT_DIR}/src/backend/main.cpp
)

set(TEST_SOURCES
    ${CORE_SOURCES}
    ${ROOT_DIR}/tests/test_main.cpp
)

add_executable(workbench_backend ${BACKEND_SOURCES})
target_include_directories(workbench_backend PRIVATE ${ROOT_DIR}/src)
if(WIN32)
    target_link_libraries(workbench_backend PRIVATE ws2_32)
endif()

add_executable(workbench_tests ${TEST_SOURCES})
target_include_directories(workbench_tests PRIVATE ${ROOT_DIR}/src)
if(WIN32)
    target_link_libraries(workbench_tests PRIVATE ws2_32)
endif()

