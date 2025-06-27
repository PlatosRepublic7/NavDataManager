add_library(NavDataManager STATIC 
    NavDataManager.cpp
    ../parser/XPlaneDatParser.cpp
    ../parser/LookaheadLineReader.cpp
)
target_include_directories(NavDataManager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../parser
)
target_link_libraries(NavDataManager PUBLIC SQLiteCpp)