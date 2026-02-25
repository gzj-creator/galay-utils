cmake_minimum_required(VERSION 3.16)
project(galay-utils VERSION 1.0.0 LANGUAGES CXX)

# Set default install prefix to /usr/local
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Default install prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

option(BUILD_MODULE_TESTS "Build C++23 module(import/export) tests" ON)

# C++23 named module support (requires CMake >= 3.28)
set(GALAY_UTILS_MODULES_SUPPORTED FALSE)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
    set(GALAY_UTILS_MODULES_SUPPORTED TRUE)
endif()

if(BUILD_MODULE_TESTS AND NOT GALAY_UTILS_MODULES_SUPPORTED)
    message(FATAL_ERROR "BUILD_MODULE_TESTS requires CMake >= 3.28")
endif()

set(GALAY_UTILS_MODULES_GENERATOR_SUPPORTED FALSE)
if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Visual Studio")
    set(GALAY_UTILS_MODULES_GENERATOR_SUPPORTED TRUE)
endif()

if(BUILD_MODULE_TESTS AND NOT GALAY_UTILS_MODULES_GENERATOR_SUPPORTED)
    message(WARNING "Generator '${CMAKE_GENERATOR}' does not support C++ modules; disabling BUILD_MODULE_TESTS.")
    set(BUILD_MODULE_TESTS OFF)
endif()

set(GALAY_UTILS_MODULES_COMPILER_SUPPORTED TRUE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(GALAY_UTILS_MODULES_COMPILER_SUPPORTED FALSE)
endif()

if(BUILD_MODULE_TESTS AND NOT GALAY_UTILS_MODULES_COMPILER_SUPPORTED)
    message(WARNING "Compiler '${CMAKE_CXX_COMPILER_ID}' does not support C++ module dependency scanning in CMake; disabling BUILD_MODULE_TESTS.")
    set(BUILD_MODULE_TESTS OFF)
endif()

if(BUILD_MODULE_TESTS AND GALAY_UTILS_MODULES_SUPPORTED AND GALAY_UTILS_MODULES_GENERATOR_SUPPORTED)
    add_library(galay-utils-modules STATIC)
    target_link_libraries(galay-utils-modules PUBLIC galay-utils)
    target_sources(galay-utils-modules
        PUBLIC
            FILE_SET CXX_MODULES
            FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/galay-utils/module/galay.utils.cppm"
    )
endif()

# Install
include(GNUInstallDirs)

install(TARGETS galay-utils
    EXPORT galay-utils-targets
)

if(TARGET galay-utils-modules)
    install(TARGETS galay-utils-modules
        EXPORT galay-utils-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/galay-utils/module
    )
endif()

if(TARGET galay-utils-modules)
    install(DIRECTORY galay-utils/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/galay-utils
        FILES_MATCHING PATTERN "*.hpp"
    )
else()
    install(DIRECTORY galay-utils/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/galay-utils
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.cppm"
    )
endif()

install(EXPORT galay-utils-targets
    FILE galay-utils-config.cmake
    NAMESPACE galay::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/galay-utils
)

option(BUILD_EXAMPLES "Build example executables" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Option to build tests
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
