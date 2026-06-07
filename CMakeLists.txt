cmake_minimum_required(VERSION 3.16)
project(galay-utils VERSION 3.1.0 LANGUAGES CXX)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)

# Set default install prefix to /usr/local
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Default install prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 构建选项 / 平台选项
include(cmake/option.cmake)

# 配置 IO 后端
configure_io_backend()

# 使用标准 BUILD_TESTING 作为测试入口；BUILD_TESTS 仅保留兼容别名。
if(DEFINED BUILD_TESTS AND NOT DEFINED BUILD_TESTING)
    set(BUILD_TESTING ${BUILD_TESTS} CACHE BOOL "Build the testing tree." FORCE)
elseif(NOT DEFINED BUILD_TESTING)
    set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree." FORCE)
endif()

if(DEFINED BUILD_TESTS)
    message(DEPRECATION "BUILD_TESTS is deprecated; use BUILD_TESTING instead.")
endif()

include(CTest)

# Header-only library
add_library(galay-utils INTERFACE)

set(GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR "" CACHE PATH
    "Directory containing concurrentqueue/moodycamel/concurrentqueue.h")

set(_GALAY_UTILS_CONCURRENTQUEUE_HINTS
    "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}"
    /opt/homebrew/include
    /usr/local/include
    /usr/include
)

if(NOT GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR)
    unset(GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR CACHE)
    find_path(GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR
        NAMES
            concurrentqueue/moodycamel/concurrentqueue.h
            concurrentqueue/moodycamel/blockingconcurrentqueue.h
        HINTS
            /opt/homebrew/include
            /usr/local/include
            /usr/include
    )
endif()

if(NOT GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR)
    find_path(_GALAY_UTILS_CONCURRENTQUEUE_PACKAGE_DIR
        NAMES
            moodycamel/concurrentqueue.h
            moodycamel/blockingconcurrentqueue.h
        HINTS ${_GALAY_UTILS_CONCURRENTQUEUE_HINTS}
        PATH_SUFFIXES concurrentqueue
    )
    if(_GALAY_UTILS_CONCURRENTQUEUE_PACKAGE_DIR)
        get_filename_component(
            GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR
            "${_GALAY_UTILS_CONCURRENTQUEUE_PACKAGE_DIR}"
            DIRECTORY
        )
        set(GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR
            "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}"
            CACHE PATH "Directory containing concurrentqueue/moodycamel/concurrentqueue.h"
            FORCE
        )
    endif()
endif()

if(GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR
   AND EXISTS "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}/moodycamel/concurrentqueue.h"
   AND NOT EXISTS "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}/concurrentqueue/moodycamel/concurrentqueue.h")
    get_filename_component(
        GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR
        "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}"
        DIRECTORY
    )
    set(GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR
        "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}"
        CACHE PATH "Directory containing concurrentqueue/moodycamel/concurrentqueue.h"
        FORCE
    )
endif()

if(NOT GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR)
    message(FATAL_ERROR
        "concurrentqueue headers not found. Set GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR "
        "to the include prefix containing concurrentqueue/moodycamel/concurrentqueue.h")
endif()

if(NOT EXISTS "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}/concurrentqueue/moodycamel/concurrentqueue.h"
   OR NOT EXISTS "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}/concurrentqueue/moodycamel/blockingconcurrentqueue.h")
    message(FATAL_ERROR
        "GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR must contain "
        "concurrentqueue/moodycamel/concurrentqueue.h and "
        "concurrentqueue/moodycamel/blockingconcurrentqueue.h")
endif()

if(NOT TARGET concurrentqueue::concurrentqueue)
    add_library(concurrentqueue::concurrentqueue INTERFACE IMPORTED GLOBAL)
    set_target_properties(concurrentqueue::concurrentqueue PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GALAY_UTILS_CONCURRENTQUEUE_INCLUDE_DIR}"
    )
endif()

target_include_directories(galay-utils
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(galay-utils INTERFACE concurrentqueue::concurrentqueue)

# Platform-specific settings
if(UNIX)
    target_link_libraries(galay-utils INTERFACE pthread)
    if(NOT APPLE)
        target_link_libraries(galay-utils INTERFACE dl)
    endif()
endif()

option(BUILD_MODULE_TESTS "Build C++23 module(import/export) tests and examples" OFF)

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
                "${CMAKE_CURRENT_SOURCE_DIR}/galay-utils/module/galay_utils.cppm"
    )
endif()

# Install
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/galay-utils-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/galay-utils-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/galay-utils"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/galay-utils-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

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

install(DIRECTORY galay-utils/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/galay-utils
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.inl"
)

install(EXPORT galay-utils-targets
    FILE galayUtilsConfigTargets.cmake
    NAMESPACE galay::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/galay-utils
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/galay-utils-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/galay-utils-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/galay-utils
)

option(BUILD_EXAMPLES "Build example executables" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(BUILD_BENCHMARKS "Build benchmark executables" OFF)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
