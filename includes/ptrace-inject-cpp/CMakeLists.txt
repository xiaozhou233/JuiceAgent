add_library(ptrace_inject STATIC
    src/library.cpp
    src/log.cpp
    src/util.cpp
    src/maps.cpp
    src/memory.cpp
    src/ptrace_utils.cpp
    src/symbols.cpp
    src/inject.cpp
    src/process_lookup.cpp
)

add_library(ptrace_inject::ptrace_inject ALIAS ptrace_inject)

target_include_directories(ptrace_inject
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(ptrace_inject PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(ptrace_inject PRIVATE dl)

set_target_properties(ptrace_inject PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)
