link_directories("$ENV{JAVA_HOME}/lib")

file(GLOB LOADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.c"
)

add_library(juiceloader SHARED ${LOADER_SOURCES})

target_include_directories(juiceloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${INCLUDES}/jvm
    ${INCLUDES}/jvm/win32
    ${INCLUDES}/JuiceLoader
)

target_link_libraries(juiceloader PRIVATE jvm)
