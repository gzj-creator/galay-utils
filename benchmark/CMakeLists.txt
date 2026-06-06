cmake_minimum_required(VERSION 3.16)
project(galay-utils-benchmark)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lru_cache_benchmark lru_cache_benchmark.cpp)
target_link_libraries(lru_cache_benchmark PRIVATE galay-utils)
