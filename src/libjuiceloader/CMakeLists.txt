file(
    GLOB_RECURSE
    libjuiceloader_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

add_library(juiceloader SHARED ${libjuiceloader_sources})

target_include_directories(juiceloader PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(juiceloader PRIVATE
    ReflectiveDLLInjection
    jvm
)  