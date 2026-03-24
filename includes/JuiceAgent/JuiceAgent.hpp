#pragma once

#include <string>

struct InjectionInfo {
    std::string JuiceAgentAPIJarPath;
    std::string JuiceAgentNativePath;

    std::string EntryJarPath;
    std::string EntryClass;
    std::string EntryMethod;
    
    std::string InjectionDir;
};