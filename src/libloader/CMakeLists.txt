file(GLOB_RECURSE loader_sources CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.c
)

if (WIN32)
    message(STATUS "Detected System: Windows")

    file(GLOB_RECURSE windows_sources CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/*.c
    )

    list(APPEND loader_sources ${windows_sources})
endif()

add_library(loader SHARED ${loader_sources})

target_include_directories(loader PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(loader PRIVATE
    jvm
    ReflectiveDLLInjection
)