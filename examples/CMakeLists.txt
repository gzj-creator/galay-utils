add_executable(E1-BasicUsage
    include/E1-basic_usage.cpp
)

target_link_libraries(E1-BasicUsage
    PRIVATE
        galay-utils
)

if(BUILD_MODULE_TESTS AND TARGET galay-utils-modules)
    add_executable(E1-BasicUsageImport
        import/E1-basic_usage.cpp
    )
    target_link_libraries(E1-BasicUsageImport
        PRIVATE
            galay-utils-modules
    )
    set_target_properties(E1-BasicUsageImport PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
