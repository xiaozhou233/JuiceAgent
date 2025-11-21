file(
    GLOB_RECURSE
    agent_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

add_library(agent SHARED ${agent_sources})

target_include_directories(agent PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32
)

target_link_libraries(agent PRIVATE
    ReflectiveDLLInjection
)  