#include <libloader.hpp>
#include <config.hpp>

namespace libloader
{
    void entrypoint(const char* runtime_dir) {
        config::Config cfg(runtime_dir);
        if (!cfg.is_valid()) {
            PLOGE << "Invalid config";
            return;
        }

        
    }
} // namespace libloader
