# injector
if(WIN32)
    add_executable(injector injector.c)
else()
    add_executable(injector injector_linux.c)
endif()
