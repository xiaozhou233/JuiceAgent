
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

# injector
add_executable(injector injector.c)

target_include_directories(injector PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32
)

target_link_libraries(injector PRIVATE
    ReflectiveDLLInjection
)