# src/utils/CMakeLists.txt

add_library(utils STATIC
    test.c)

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_LIST_DIR})