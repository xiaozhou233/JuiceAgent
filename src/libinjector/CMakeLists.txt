file(
    GLOB_RECURSE
    libinjector_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

add_library(injector SHARED ${libinjector_sources})

target_include_directories(injector PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32
)

target_link_libraries(injector PRIVATE
    ReflectiveDLLInjection
)