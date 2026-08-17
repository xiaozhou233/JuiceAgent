#include <services/base.hpp>
#include <services/service_manager.hpp>
#include <event/event_type.hpp>
#include <JuiceAgent/Logger.hpp>

namespace JuiceAgent {
namespace Services {

class BytecodeService : public IService {
public:
    const char* name() const override { return "BytecodeService"; }

    bool onInitialize() override {
        listen<BytecodeService, EventClassFileLoadHook>(
            EventId::ClassFileLoadHook,
            &BytecodeService::captureBytecodes
        );

        listen<BytecodeService, EventClassFileLoadHook>(
            EventId::ClassFileLoadHook,
            &BytecodeService::patchBytecodes
        );

        spdlog::info("[BytecodeService] Initialized");
        return true;
    }

    void onShutdown() override {
        spdlog::info("[BytecodeService] Shutdown");
    }

private:
    void captureBytecodes(const EventClassFileLoadHook& e) {
        // TODO: Capture original bytes
        spdlog::trace("[BytecodeService] Capture original bytes: {}",
                      e.name ? e.name : "<null>");
    }

    void patchBytecodes(const EventClassFileLoadHook& e) {
        // TODO: Apply patches
        spdlog::trace("[BytecodeService] Apply patches: {}",
                      e.name ? e.name : "<null>");
    }
};

}
}

JUICEAGENT_REGISTER_SERVICE(::JuiceAgent::Services::BytecodeService)
