cmake_minimum_required(VERSION 3.16)
project(galay-utils-test)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add parent directory for includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(GALAY_UTILS_TEST_GROUPS
    algorithm/algorithm_test.cpp
    app/app_test.cpp
    buffer/buffer_test.cpp
    cache/cache_test.cpp
    concurrency/concurrency_test.cpp
    core/core_test.cpp
    data/data_test.cpp
    platform/platform_test.cpp
    resilience/resilience_test.cpp
    routing/routing_test.cpp
)

foreach(test_source IN LISTS GALAY_UTILS_TEST_GROUPS)
    get_filename_component(test_name "${test_source}" NAME_WE)
    add_executable("${test_name}" "${test_source}")
    target_link_libraries("${test_name}" PRIVATE galay-utils)
    if(test_name STREQUAL "concurrency_test" OR
       test_name STREQUAL "cache_test" OR
       test_name STREQUAL "buffer_test")
        target_compile_definitions("${test_name}" PRIVATE
            GALAY_UTILS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
        )
    endif()
    add_test(NAME "${test_name}" COMMAND "${test_name}")
endforeach()

if(BUILD_MODULE_TESTS AND TARGET galay-utils-modules)
    add_executable(test_import_smoke import_smoke.cpp)
    target_link_libraries(test_import_smoke PRIVATE galay-utils-modules)
    set_target_properties(test_import_smoke PROPERTIES CXX_SCAN_FOR_MODULES ON)

    add_test(NAME ModuleImportSmoke COMMAND test_import_smoke)
endif()
