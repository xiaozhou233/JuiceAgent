file(
    GLOB_RECURSE
    agent_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(agent SHARED ${agent_sources})

target_include_directories(agent PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32
)