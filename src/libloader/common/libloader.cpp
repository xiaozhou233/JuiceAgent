#include <JuiceAgent/Logger.hpp>
#include <libloader.hpp>
#include <config.hpp>
#include <jvm.hpp>

namespace libloader
{
    void entrypoint(const char* runtime_dir)
    {
        if (runtime_dir == nullptr) {
            PLOGE << "runtime_dir is null, fallback may fail";
        }

        // Load Config (allow fallback)
        config::Config cfg(runtime_dir);
        if (!cfg.is_valid()) {
            PLOGW << "Invalid config, using default values";
        }

        InjectionInfo info = cfg.get_injection_info();
        config::print_injection_info(info);

        // Validate injection path early
        if (info.JuiceAgentAPIJarPath.empty()) {
            PLOGE << "JuiceAgentAPIJarPath is empty, abort injection";
            return;
        }

        // Attach to JVM
        jvm::Jvm jvm;
        if (!jvm.attach()) {
            PLOGE << "Attach to JVM failed";
            return;
        }

        auto* jvmti = jvm.get_jvmti();
        if (jvmti == nullptr) {
            PLOGE << "JVMTI is null";
            return;
        }

        // Inject JuiceAgentAPI.jar
        const char* jar_path = info.JuiceAgentAPIJarPath.c_str();
        jint status = jvmti->AddToSystemClassLoaderSearch(jar_path);

        if (status != JNI_OK) {
            PLOGE << "AddToSystemClassLoaderSearch failed: " << status;
            return;
        }

        PLOGI << "Jar injected successfully: " << jar_path;
    }
} // namespace libloader