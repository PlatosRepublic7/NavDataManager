cmake_minimum_required(VERSION 3.20)  # Bump to 3.20 for better C++17 support
project(NavDataManager 
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "X-Plane Navigation Data Management Library"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions

# Add compile features for better compatibility
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Modern way to handle build types
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(FetchContent)
include(GNUInstallDirs)  # For standard install directories

# Fetch SQLiteCpp with version pinning (more reliable than master)
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.1  # Pin to specific version instead of master
    GIT_SHALLOW    TRUE   # Faster clone
)

# Configure SQLiteCpp options before making available
set(SQLITECPP_FETCH_SQLITE ON CACHE BOOL "")
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "")
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "")

FetchContent_MakeAvailable(SQLiteCpp)

# Only fetch GoogleTest if testing is enabled
option(BUILD_TESTING "Build the testing tree." ON)
if(BUILD_TESTING)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0  # Use latest stable version
        GIT_SHALLOW    TRUE
    )
    
    # Prevent GoogleTest from overriding compiler/linker options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# Set path to cmake utilities
set(CMAKE_UTILS_DIR "${CMAKE_SOURCE_DIR}/cmake")

# Add the main library
add_subdirectory(src)

# Optional development driver
option(BUILD_DRIVER "Build the development driver executable" OFF)
if(BUILD_DRIVER)
    add_executable(mdriver src/main.cpp)
    target_link_libraries(mdriver PRIVATE NavDataManager::NavDataManager)
endif()

# Tests
if(BUILD_TESTING)
    add_subdirectory(test)
endif()