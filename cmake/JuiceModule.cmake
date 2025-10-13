# JuiceModule.cmake
# Function to add a JuiceAgent module (agent, injector, juiceloader)

function(add_juice_module NAME)
    link_directories("$ENV{JAVA_HOME}/lib")

    # Collect all source files in module src and utils
    file(GLOB MODULE_SOURCES
        "${CMAKE_SOURCE_DIR}/src/${NAME}/*.c"
        "${CMAKE_SOURCE_DIR}/src/${NAME}/utils/*.c"
    )

    # Create shared library
    add_library(${NAME} SHARED ${MODULE_SOURCES})

    # Include directories
    target_include_directories(${NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/includes
        ${CMAKE_SOURCE_DIR}/includes/jvm
        ${CMAKE_SOURCE_DIR}/includes/jvm/win32
        ${CMAKE_SOURCE_DIR}/includes/JuiceAgent
        ${CMAKE_SOURCE_DIR}/includes/ReflectiveDLLInjection
        ${CMAKE_SOURCE_DIR}/includes/log.c
        "${CMAKE_SOURCE_DIR}/src/${NAME}/utils"
        "${CMAKE_SOURCE_DIR}/src/${NAME}/include"
        "${CMAKE_SOURCE_DIR}/src/utils"
    )

    # Link common libraries
    target_link_libraries(${NAME} PRIVATE
        ReflectiveDLLInjection
        log.c
        jvm
        utils
    )

    add_subdirectory("${CMAKE_SOURCE_DIR}/src/${NAME}")
endfunction()
