cmake_minimum_required(VERSION 3.16)
project(galay-utils-benchmark)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lru_cache_benchmark lru_cache_benchmark.cpp)
target_link_libraries(lru_cache_benchmark PRIVATE galay-utils)

add_executable(byte_queue_view_benchmark byte_queue_view_benchmark.cpp)
target_link_libraries(byte_queue_view_benchmark PRIVATE galay-utils)

add_executable(ring_buffer_benchmark ring_buffer_benchmark.cpp)
target_link_libraries(ring_buffer_benchmark PRIVATE galay-utils)

add_executable(bloom_filter_benchmark bloom_filter_benchmark.cpp)
target_link_libraries(bloom_filter_benchmark PRIVATE galay-utils)
