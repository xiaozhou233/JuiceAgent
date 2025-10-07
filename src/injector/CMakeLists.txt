link_directories("$ENV{JAVA_HOME}/lib")

file(GLOB INJECTOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.c"
)

add_library(injector SHARED ${INJECTOR_SOURCES})

target_include_directories(injector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${INCLUDES}/jvm
    ${INCLUDES}/jvm/win32
    ${INCLUDES}/JuiceLoader
)

target_link_libraries(injector PRIVATE
    jvm
    ReflectiveDLLInjection
)
