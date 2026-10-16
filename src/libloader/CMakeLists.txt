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
    ${CMAKE_SOURCE_DIR}/includes/jvm/win32
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(loader PRIVATE
    jvm
    ReflectiveDLLInjection
)