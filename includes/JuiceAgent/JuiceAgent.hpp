#pragma once

#include <string>

struct InjectionInfo {
    std::string JuiceAgentAPIJarPath;
    std::string JuiceAgentNativeLibraryPath;

    std::string ConfigDir;
};