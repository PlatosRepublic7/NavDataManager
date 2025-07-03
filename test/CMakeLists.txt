# Collect test files
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/create_test_db.cpp")

# Create test executable
add_executable(run_tests ${TEST_SOURCES})

# Create database creator executable
add_executable(create_test_db create_test_db.cpp)

# Link against library and test framework
target_link_libraries(run_tests 
    PRIVATE 
        NavDataManager::NavDataManager
        GTest::gtest_main
)

# Link database creator
target_link_libraries(create_test_db
    PRIVATE
        NavDataManager::NavDataManager
)

# Set test properties
set_target_properties(run_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include current directory for test headers
target_include_directories(run_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Discover and register tests
include(GoogleTest)
gtest_discover_tests(run_tests)