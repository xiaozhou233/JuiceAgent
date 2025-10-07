add_library(ReflectiveDLLInjection STATIC
    GetProcAddressR.c
    LoadLibraryR.c
    ReflectiveLoader.c
)

target_include_directories(ReflectiveDLLInjection PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
