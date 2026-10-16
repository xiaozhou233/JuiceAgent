#include <Loader.hpp>
#include <JvmAttach.hpp>
#include <JuiceAgent/Logger.hpp>

void JuiceAgent::Loader::entrypoint(const char* path) {
    // Called from DllMain inside the target JVM process. NOTE: libloader is
    // reflectively loaded, so a C++ exception thrown here cannot be unwound and
    // would crash the target process - this code must not throw (see the note
    // in Loader::preload).

    // Attach to the JVM
    JuiceAgent::Loader::JvmAttach jvmAttach;

    if(!jvmAttach.attach()) {
        spdlog::error("Failed to attach to JVM!");
        return;
    }

    // Get JNIEnv
    JNIEnv* env = jvmAttach.get_env();
    if(!env) {
        spdlog::error("Failed to get JNIEnv!");
        return;
    }

    // Get JVMTI environment
    jvmtiEnv* jvmti = jvmAttach.get_jvmti();
    if(!jvmti) {
        spdlog::error("Failed to get JVMTI environment!");
        return;
    }
    
    JuiceAgent::Loader::preload(path, env, jvmti);
}
