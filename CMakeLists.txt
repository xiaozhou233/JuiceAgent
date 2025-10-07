cmake_minimum_required(VERSION 3.15)
project(JuiceAgent LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# includes
set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/includes)

# library - ReflectiveDLLInjection
add_subdirectory(${INCLUDES}/ReflectiveDLLInjection)

include(cmake/JuiceModule.cmake)

# modules
add_juice_module(agent)
add_juice_module(juiceloader)
add_juice_module(injector)