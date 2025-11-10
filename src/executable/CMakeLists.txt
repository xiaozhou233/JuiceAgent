# src/executable/CMakeLists.txt

add_executable(injector_executable injector.c)

target_include_directories(injector_executable PRIVATE
    ${CMAKE_SOURCE_DIR}/includes
    ${CMAKE_SOURCE_DIR}/includes/JuiceAgent
)