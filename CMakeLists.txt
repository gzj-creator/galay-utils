cmake_minimum_required(VERSION 3.16)
project(galay-utils VERSION 1.0.0 LANGUAGES CXX)

# Set default install prefix to /usr/local
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Default install prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only library
add_library(galay-utils INTERFACE)

target_include_directories(galay-utils
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Platform-specific settings
if(UNIX)
    target_link_libraries(galay-utils INTERFACE pthread)
    if(NOT APPLE)
        target_link_libraries(galay-utils INTERFACE dl)
    endif()
endif()

# Install
include(GNUInstallDirs)

install(TARGETS galay-utils
    EXPORT galay-utils-targets
)

install(DIRECTORY galay-utils/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/galay-utils
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT galay-utils-targets
    FILE galay-utils-config.cmake
    NAMESPACE galay::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/galay-utils
)

# Option to build tests
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
