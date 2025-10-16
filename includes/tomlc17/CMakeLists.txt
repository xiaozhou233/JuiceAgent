# includes/tomlc17/CMakeLists.txt

add_library(tomlc17 STATIC tomlc17.c)
set_target_properties(tomlc17 PROPERTIES
    OUTPUT_NAME "tomlc17"
)

target_include_directories(tomlc17 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)