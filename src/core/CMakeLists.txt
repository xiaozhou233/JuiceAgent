link_directories("$ENV{JAVA_HOME}/lib")

file(GLOB CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.c"
)

add_library(agent SHARED ${CORE_SOURCES})

target_include_directories(agent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${INCLUDES}/jvm
    ${INCLUDES}/jvm/win32
    ${INCLUDES}/JuiceLoader
)

target_compile_definitions(agent PRIVATE REFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR)

target_link_libraries(agent PRIVATE
    ReflectiveDLLInjection
    jvm
)
