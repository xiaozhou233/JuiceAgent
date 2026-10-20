# injector
if(WIN32)
    add_executable(injector injector.c)
else()
    add_executable(injector injector_linux.cpp)
    target_link_libraries(injector PRIVATE ptrace_inject::ptrace_inject)
endif()
