#pragma once

#include <JuiceAgent/JuiceAgent.hpp>
#include <string>
#include <JuiceAgent/Logger.hpp>
#include <jvm/jni.h>
#include <jvm/jvmti.h>

namespace libloader
{
    void entrypoint(const char* runtime_dir);    

    bool invoke_juiceagent_init(JNIEnv* env, const InjectionInfo& info);
} // namespace libloader
