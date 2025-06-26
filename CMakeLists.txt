cmake_minimum_required(VERSION 3.16)
project(NavDataManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch SQLiteCpp from GitHub
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        master # Or use a specific release tag, e.g., 3.3.1
)

# We need to prevent runtime library conflicts on Windows
if(MSVC)
    set(SQLITECPP_STATIC_RUNTIMELIBRARY_FLAG "/MT")
endif()

set(SQLITECPP_FETCH_SQLITE ON CACHE BOOL "")

if(WIN32)
    set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "")
endif()

FetchContent_MakeAvailable(SQLiteCpp)

# Fetch Google Test
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG    release-1.11.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()

# Add subdirectories for each component
add_subdirectory(src/navlib)

# Driver executable for development/testing
add_executable(mdriver src/main.cpp)
target_link_libraries(mdriver PRIVATE NavDataManager)

# Link the text executable against the NavDataManager library and GoogleTest
file(GLOB TEST_SRC CONFIGURE_DEPENDS test/*.cpp)

message(STATUS "TEST_SRC = ${TEST_SRC}")
add_executable(run_tests ${TEST_SRC})
target_link_libraries(run_tests PRIVATE NavDataManager GTest::gtest_main)

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(run_tests)