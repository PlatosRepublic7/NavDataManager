# Generate schema header
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sql/schema.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/sql
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_SCHEMA=${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql
        -DOUTPUT_HEADER=${CMAKE_CURRENT_BINARY_DIR}/sql/schema.h
        -P ${CMAKE_UTILS_DIR}/generate_schema_header.cmake
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql 
        ${CMAKE_UTILS_DIR}/generate_schema_header.cmake
    COMMENT "Generating schema.h from schema.sql"
    VERBATIM  # Proper argument escaping
)

add_custom_target(schema_header DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sql/schema.h)

# Main library target
add_library(NavDataManager
    # Core library files
    navlib/NavDataManager.cpp
    
    # Parser files  
    parser/XPlaneDatParser.cpp
    parser/LookaheadLineReader.cpp
    
    # Future query files
    navlib/AirportQuery.cpp
    # navlib/RunwayQuery.cpp
    # navlib/NavaidQuery.cpp
    
    # Generated files
    ${CMAKE_CURRENT_BINARY_DIR}/sql/schema.h
)

# Modern target dependencies
add_dependencies(NavDataManager schema_header)

# Create an alias for consistent naming
add_library(NavDataManager::NavDataManager ALIAS NavDataManager)

# Modern include directory handling
target_include_directories(NavDataManager
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/sql  # For generated schema.h
        ${CMAKE_CURRENT_SOURCE_DIR}      # For internal headers
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
)

# Link libraries with modern interface
target_link_libraries(NavDataManager 
    PUBLIC 
        SQLiteCpp  # Public because users might need SQLite types
)

# Compiler-specific options
target_compile_options(NavDataManager PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# Set properties for better IDE support
set_target_properties(NavDataManager PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME NavDataManager
)

# Installation rules
install(TARGETS NavDataManager
    EXPORT NavDataManagerTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/NavDataManager
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Export targets for find_package support
install(EXPORT NavDataManagerTargets
    FILE NavDataManagerTargets.cmake
    NAMESPACE NavDataManager::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NavDataManager
)

# Generate config files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/NavDataManagerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NavDataManagerConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NavDataManager
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NavDataManagerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/NavDataManagerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/NavDataManagerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NavDataManager
)