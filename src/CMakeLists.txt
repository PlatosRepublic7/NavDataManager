add_library(NavDataManager STATIC 
    navlib/NavDataManager.cpp
    parser/XPlaneDatParser.cpp
    parser/LookaheadLineReader.cpp
)
target_include_directories(NavDataManager 
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/navlib
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
)

target_link_libraries(NavDataManager PUBLIC SQLiteCpp)