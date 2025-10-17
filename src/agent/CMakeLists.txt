# src/core/CMakeLists.txt

target_include_directories(agent PRIVATE ${CMAKE_SOURCE_DIR}/includes/tomlc17)

target_link_libraries(agent PRIVATE shlwapi tomlc17)