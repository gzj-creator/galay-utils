add_executable(E1-BasicUsage
    include/E1-basic_usage.cpp
)

target_link_libraries(E1-BasicUsage
    PRIVATE
        galay-utils
)
