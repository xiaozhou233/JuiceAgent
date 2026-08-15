file(
    GLOB_RECURSE
    agent_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

if (agent_sources)
    add_library(agent SHARED ${agent_sources})

    target_include_directories(agent PRIVATE
        ${CMAKE_SOURCE_DIR}/includes
        ${CMAKE_SOURCE_DIR}/includes/jvm
        ${CMAKE_SOURCE_DIR}/includes/jvm/win32
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
    )

    target_link_libraries(agent PRIVATE
        jvm
    )
endif()
