#include <JuiceAgent/Logger.hpp>
#include <libloader.hpp>
#include <config.hpp>
#include <jvm.hpp>

namespace libloader
{
    void entrypoint(const char* runtime_dir) {

        // Load Config
        config::Config cfg(runtime_dir);
        if (!cfg.is_valid()) {
            PLOGE << "Invalid config";
        }

        InjectionInfo info = cfg.get_injection_info();
        config::print_injection_info(info);

        // Attach to JVM
        jvm::Jvm jvm;

        if (!jvm.attach()) {
            PLOGE << "Attach failed";
            return;
        }

        // Inject JuiceLoaderAPI.jar
        jint status = jvm.get_jvmti()->AddToSystemClassLoaderSearch(info.JuiceAgentAPIJarPath.c_str());
        if (status != JNI_OK) {
            PLOGE << "AddToSystemClassLoaderSearch failed: " << status;
        }

    }
} // namespace libloader
