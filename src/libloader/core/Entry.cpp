#include <exception>

#include <Loader.hpp>
#include <JvmAttach.hpp>
#include <JuiceAgent/Logger.hpp>

void JuiceAgent::Loader::entrypoint(const char* path) {
    // This runs from DllMain inside the target JVM process, so a C++ exception
    // escaping here would take the target process down. Contain everything as a
    // last line of defence; a bad config.toml is reported and logged instead.
    try {
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
    } catch (const std::exception& e) {
        spdlog::error("[Entry] Unhandled exception in entrypoint: {}", e.what());
    } catch (...) {
        spdlog::error("[Entry] Unhandled unknown exception in entrypoint");
    }
}
