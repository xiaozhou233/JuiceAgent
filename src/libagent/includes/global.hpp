#pragma once

#include <jni.h>
#include <jvmti.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

// Data Structures
struct ClassFileData {
        std::string classname;
        std::vector<unsigned char> bytecode;

        // ======== Global references to prevent GC ========
        // Remember to release these references when they are no longer needed, otherwise it will cause memory leaks
        jclass clazz;
        jobject classloader;
        jobject protection_domain;
        
};

// Set of target class internal names to capture (e.g. java/lang/String)
extern std::unordered_set<std::string> classToCapture;

// Cache of captured class bytecode, keyed by internal class name
extern std::unordered_map<std::string, ClassFileData> classFileDataMap;

// Mutex protecting access to classToCapture and classFileDataMap (thread-safe for JVMTI callbacks)
extern std::mutex classDataMutex;