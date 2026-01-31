cmake_minimum_required(VERSION 3.16)
project(galay-utils-test)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add parent directory for includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Test executable
add_executable(test_all test_all.cpp)

# Link pthread and galay-kernel on Unix
if(UNIX)
    target_link_libraries(test_all pthread galay-kernel)
    if(NOT APPLE)
        target_link_libraries(test_all dl)
    endif()
endif()

# Enable testing
enable_testing()
add_test(NAME AllTests COMMAND test_all)
