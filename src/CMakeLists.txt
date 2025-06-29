add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/sql/schema.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/sql
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_SCHEMA=${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql
        -DOUTPUT_HEADER=${CMAKE_BINARY_DIR}/sql/schema.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_schema_header.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql ${CMAKE_CURRENT_SOURCE_DIR}/generate_schema_header.cmake
    COMMENT "Generating schema.h from schema.sql"
)

add_custom_target(schema_header DEPENDS ${CMAKE_BINARY_DIR}/sql/schema.h)

add_library(NavDataManager STATIC 
    navlib/NavDataManager.cpp
    parser/XPlaneDatParser.cpp
    parser/LookaheadLineReader.cpp
)

add_dependencies(NavDataManager schema_header)

target_include_directories(NavDataManager 
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_BINARY_DIR}/sql
        ${CMAKE_CURRENT_SOURCE_DIR}/navlib
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
)

target_link_libraries(NavDataManager PUBLIC SQLiteCpp)