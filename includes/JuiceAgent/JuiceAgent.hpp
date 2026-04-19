#pragma once

#include <string>

struct LoaderConfig {
    std::string JuiceAgentAPIJarPath;
    std::string JuiceAgentNativeLibraryPath;

    std::string RuntimeDir;
};