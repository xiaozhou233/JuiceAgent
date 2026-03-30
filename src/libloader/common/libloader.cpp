#include <JuiceAgent/Logger.hpp>
#include <libloader.hpp>
#include <config.hpp>
#include <jvm.hpp>

namespace libloader
{
    void entrypoint(const char* runtime_dir) {
        config::Config cfg(runtime_dir);
        if (!cfg.is_valid()) {
            PLOGE << "Invalid config";
        }

        InjectionInfo info = cfg.get_injection_info();
        config::print_injection_info(info);

        jvm::Jvm jvm;

        if (!jvm.attach()) {
            PLOGE << "Attach failed";
            return;
        }
    }
} // namespace libloader
