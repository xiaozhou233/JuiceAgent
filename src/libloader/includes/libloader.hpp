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

namespace {
    // RAII wrapper for local JNI references
    template<typename T>
    class LocalRef {
    private:
        JNIEnv* env;
        T ref;
    public:
        LocalRef(JNIEnv* env, T ref) : env(env), ref(ref) {}
        ~LocalRef() {
            if (ref) env->DeleteLocalRef(ref);
        }
        T get() const { return ref; }
        operator T() const { return ref; }
    };
}