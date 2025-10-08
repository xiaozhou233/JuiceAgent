add_library(log.c STATIC
    log.c
)

target_include_directories(log.c PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
