cmake_minimum_required(VERSION 3.15)
project(JuiceAgent LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

if(WIN32)
    link_directories("${CMAKE_SOURCE_DIR}/lib")
endif()

# libraries
if (WIN32)
    add_subdirectory(${CMAKE_SOURCE_DIR}/includes/ReflectiveDLLInjection)
else()
    set(PTRACE_INJECT_BUILD_CLI OFF CACHE BOOL "" FORCE)
    set(PTRACE_INJECT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    add_subdirectory(ptrace-inject-cpp)
endif()

# modules
add_subdirectory(${SRC_DIR}/libloader)
add_subdirectory(${SRC_DIR}/injector)
if(WIN32)
    add_subdirectory(${SRC_DIR}/libinject)
endif()

add_subdirectory(${SRC_DIR}/libagent)
