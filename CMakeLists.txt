cmake_minimum_required(VERSION 3.15)
project(JuiceAgent LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# version
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 3)
set(PROJECT_BUILD_NUMBER 36)
set(PROJECT_VERSION_STRING "v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}-b${PROJECT_BUILD_NUMBER}")
message(STATUS " ======== Build version: ${PROJECT_VERSION_STRING} ======== ")

add_compile_definitions(
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_BUILD_NUMBER=${PROJECT_BUILD_NUMBER}
    PROJECT_VERSION_STRING=\"${PROJECT_VERSION_STRING}\")

# output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# includes
set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/includes)

# libraries
add_subdirectory(${INCLUDES}/ReflectiveDLLInjection)
add_subdirectory(${INCLUDES}/log.c)
add_subdirectory(${INCLUDES}/tomlc17)

include(cmake/JuiceModule.cmake)

# modules
add_juice_module(agent)
add_juice_module(injector)