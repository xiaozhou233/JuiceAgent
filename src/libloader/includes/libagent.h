#ifndef LIBAGENT_H
#define LIBAGENT_H

#include <stdbool.h>

extern "C" __declspec(dllexport)
bool InitJuiceAgent(const char* runtime_path);


#endif // LIBAGENT_H