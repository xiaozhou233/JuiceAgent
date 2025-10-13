# src/utils/CMakeLists.txt

add_library(utils STATIC
    JVMUtils.c)

target_include_directories(utils PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32)

target_link_libraries(utils PRIVATE
    jvm)