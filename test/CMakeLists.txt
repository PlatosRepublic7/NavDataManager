# Collect test files
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Create test executable
add_executable(run_tests ${TEST_SOURCES})

# Link against library and test framework
target_link_libraries(run_tests 
    PRIVATE 
        NavDataManager::NavDataManager
        GTest::gtest_main
)

# Set test properties
set_target_properties(run_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Discover and register tests
include(GoogleTest)
gtest_discover_tests(run_tests)