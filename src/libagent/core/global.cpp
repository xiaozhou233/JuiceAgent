#include <global.hpp>

std::unordered_set<std::string> classToCapture;
std::unordered_map<std::string, ClassFileData> classFileDataMap;
std::mutex classDataMutex;
