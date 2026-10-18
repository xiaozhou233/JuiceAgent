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
else()
    message(STATUS "Detected System: Linux or Other")

    file(GLOB_RECURSE linux_sources CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux/*.c
    )

    list(APPEND loader_sources ${linux_sources})
endif()

add_library(loader SHARED ${loader_sources})

# libloader is reflectively loaded (manually mapped), so the OS never registers
# its unwind tables: a C++ exception thrown here cannot be caught and would crash
# the target process. Disable exceptions so such code fails the build instead.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(loader PRIVATE -fno-exceptions)
    target_compile_definitions(loader PRIVATE SPDLOG_NO_EXCEPTIONS)
endif()

target_include_directories(loader PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/jvm
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

if(WIN32)
    target_include_directories(loader PRIVATE
        ${CMAKE_SOURCE_DIR}/includes/jvm/win32
    )

    target_link_libraries(loader PRIVATE jvm ReflectiveDLLInjection)
else()
    target_include_directories(loader PRIVATE
        ${CMAKE_SOURCE_DIR}/includes/jvm/linux
    )
endif()