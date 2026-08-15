
# libinject
add_library(inject SHARED libinject.c)

target_include_directories(inject PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32
)

target_link_libraries(inject PRIVATE
    ReflectiveDLLInjection
)
