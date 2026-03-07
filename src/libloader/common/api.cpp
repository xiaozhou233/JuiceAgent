// for debug tools or other stuff

#include <libloader.hpp>

extern "C" __declspec(dllexport)
bool InitJuiceAgent(const char* runtime_path) {
    libloader::entrypoint(runtime_path);
    return true;
}