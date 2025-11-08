# src/agent/CMakeLists.txt

target_include_directories(agent PRIVATE ${CMAKE_SOURCE_DIR}/includes/tomlc17)

target_link_libraries(agent PRIVATE tomlc17)
