#pragma once

#include <jvm/jni.h>
#include <jvm/jvmti.h>

namespace JuiceAgent::Loader {
    void entrypoint(const char* runtime_dir);
    void preload(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti);
    void initialize(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti);
} // namespace JuiceAgent::Loader
