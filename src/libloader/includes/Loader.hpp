#pragma once

#include <jvm/jni.h>
#include <jvm/jvmti.h>
#include <JuiceAgent/Config.hpp>

namespace JuiceAgent::Loader {
    void entrypoint(const char* runtime_dir);
    void preload(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti);
    void initialize(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti, JuiceAgent::Config::Config& cfg);

    int load_builtin(jvmtiEnv* jvmti);
    int load_custom(jvmtiEnv* jvmti, const char* jar_path);
} // namespace JuiceAgent::Loader
