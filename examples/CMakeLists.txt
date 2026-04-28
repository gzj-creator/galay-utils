add_executable(E1-BasicUsage
    include/e1_basic.cpp
)

target_link_libraries(E1-BasicUsage
    PRIVATE
        galay-utils
)

if(BUILD_MODULE_TESTS AND TARGET galay-utils-modules)
    add_executable(E1-BasicUsageImport
        import/e1_basic.cpp
    )
    target_link_libraries(E1-BasicUsageImport
        PRIVATE
            galay-utils-modules
    )
    set_target_properties(E1-BasicUsageImport PROPERTIES CXX_SCAN_FOR_MODULES ON)
endif()
