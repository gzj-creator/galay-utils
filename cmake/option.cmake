# galay-utils 构建选项配置

# 设置默认安装目录为/usr/local
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr/local CACHE PATH "Install path prefix" FORCE)
endif()

# 库类型选项（CMake 标准变量）
option(BUILD_SHARED_LIBS "Build shared library instead of static" ON)

# IO后端选项
option(DISABLE_IOURING "Disable io_uring and use epoll on Linux" ON)

# 平台特定的异步IO后端检测
function(configure_io_backend)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(DISABLE_IOURING)
            message(STATUS "io_uring disabled by user, using epoll")
            add_compile_definitions(USE_EPOLL)
            set(GALAY_UTILS_BACKEND "epoll" PARENT_SCOPE)
        else()
            find_library(URING_LIB uring)
            if(URING_LIB)
                message(STATUS "Found liburing: ${URING_LIB}, using io_uring")
                add_compile_definitions(USE_IOURING)
                set(GALAY_UTILS_BACKEND "io_uring" PARENT_SCOPE)
                set(PLATFORM_LIBS ${URING_LIB} PARENT_SCOPE)
            else()
                message(STATUS "liburing not found, falling back to epoll")
                add_compile_definitions(USE_EPOLL)
                set(GALAY_UTILS_BACKEND "epoll" PARENT_SCOPE)
            endif()
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        message(STATUS "macOS detected, using kqueue")
        add_compile_definitions(USE_KQUEUE)
        set(GALAY_UTILS_BACKEND "kqueue" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        message(STATUS "Windows detected, using IOCP")
        add_compile_definitions(USE_IOCP)
        set(GALAY_UTILS_BACKEND "iocp" PARENT_SCOPE)
    else()
        message(WARNING "Unknown platform: ${CMAKE_SYSTEM_NAME}")
        set(GALAY_UTILS_BACKEND "unknown" PARENT_SCOPE)
    endif()
endfunction()
