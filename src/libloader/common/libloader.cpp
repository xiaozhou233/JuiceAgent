#include <JuiceAgent/Logger.hpp>
#include <libloader.hpp>
#include <config.hpp>

namespace libloader
{
    void entrypoint(const char* runtime_dir) {
        config::Config cfg(runtime_dir);
        if (!cfg.is_valid()) {
            PLOGE << "Invalid config";
        }

        InjectionInfo info = cfg.get_injection_info();
        config::print_injection_info(info);
    }
} // namespace libloader
