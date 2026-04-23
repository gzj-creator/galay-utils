cmake_minimum_required(VERSION 3.16)
project(galay-utils VERSION 1.2.1 LANGUAGES CXX)

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
                "${CMAKE_CURRENT_SOURCE_DIR}/galay-utils/module/galay.utils.cppm"
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
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT galay-utils-targets
    FILE galay-utils-targets.cmake
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

if(BUILD_TESTING)
    set(GALAY_UTILS_LOCAL_KERNEL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../galay-kernel/install-local")
    if(EXISTS "${GALAY_UTILS_LOCAL_KERNEL_PREFIX}/lib/cmake/galay-kernel/galay-kernel-config.cmake")
        set(galay-kernel_DIR
            "${GALAY_UTILS_LOCAL_KERNEL_PREFIX}/lib/cmake/galay-kernel"
            CACHE PATH "Preferred local galay-kernel package directory" FORCE)
        list(PREPEND CMAKE_PREFIX_PATH "${GALAY_UTILS_LOCAL_KERNEL_PREFIX}")
        list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
        if(EXISTS "${GALAY_UTILS_LOCAL_KERNEL_PREFIX}/lib")
            set(CMAKE_BUILD_RPATH "${GALAY_UTILS_LOCAL_KERNEL_PREFIX}/lib")
            set(CMAKE_INSTALL_RPATH "${GALAY_UTILS_LOCAL_KERNEL_PREFIX}/lib")
        endif()
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
    endif()

    find_package(galay-kernel 3.4.4 CONFIG REQUIRED)
    add_subdirectory(test)
endif()
