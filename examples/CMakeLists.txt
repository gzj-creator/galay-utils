add_executable(E1-BasicUsage
    include/E1-BasicUsage.cpp
)

target_link_libraries(E1-BasicUsage
    PRIVATE
        galay-utils
)
