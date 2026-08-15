#pragma once

#include <string>

namespace JuiceAgent {

struct LoaderConfig {
    std::string JuiceAgentNativeLibraryPath;
    std::string RuntimeDir;
};

} // namespace JuiceAgent
