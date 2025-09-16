cmake_minimum_required(VERSION 3.15) 

project(juiceagent LANGUAGES CXX C) 

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src) 
set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/includes)

# ReflectiveDLLInjection
add_library(ReflectiveDLLInjection STATIC 
    ${INCLUDES}/ReflectiveDLLInjection/GetProcAddressR.c
    ${INCLUDES}/ReflectiveDLLInjection/LoadLibraryR.c
    ${INCLUDES}/ReflectiveDLLInjection/ReflectiveLoader.c
    )

# libagent
link_directories("$ENV{JAVA_HOME}/lib")
add_library(agent SHARED ${SRC}/libagent.c)
target_include_directories(agent PRIVATE 
    ${INCLUDES}/ReflectiveDLLInjection
    ${INCLUDES}/jvm
    ${INCLUDES}/jvm/win32)
target_link_libraries(agent PRIVATE ReflectiveDLLInjection)
target_link_libraries(agent PRIVATE jvm)

# libjuiceloader
link_directories("$ENV{JAVA_HOME}/lib")
add_library(juiceloader SHARED ${SRC}/libjuiceloader.c)
target_include_directories(juiceloader PRIVATE 
    ${INCLUDES}/jvm
    ${INCLUDES}/jvm/win32
    ${INCLUDES}/JuiceLoader)
target_link_libraries(juiceloader PRIVATE jvm)