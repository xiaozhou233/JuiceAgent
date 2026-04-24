#include <global.hpp>

std::unordered_set<std::string> classToCapture;
std::unordered_map<std::string, ClassFileData> classFileDataMap;
std::mutex classDataMutex;

std::unordered_map<std::string, std::vector<unsigned char>> pendingRetransform;
std::mutex pendingRetransformMutex;

std::unordered_set<std::string> remapperWhiteList;