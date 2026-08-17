#include <services/bytecode_service.hpp>
#include <JuiceAgent/Logger.hpp>

namespace JuiceAgent {
namespace Services {

BytecodeService& BytecodeService::getInstance() {
    static BytecodeService instance;
    return instance;
}

bool BytecodeService::onInitialize() {
    listen<BytecodeService, EventClassFileLoadHook>(
        EventId::ClassFileLoadHook,
        &BytecodeService::onClassFileLoad
    );

    spdlog::info("[BytecodeService] Initialized");
    return true;
}

void BytecodeService::onShutdown() {
    spdlog::info("[BytecodeService] Shutdown");
}

void BytecodeService::onClassFileLoad(const EventClassFileLoadHook& event) {
    spdlog::trace("[BytecodeService] ClassFileLoadHook: {}",
                  event.name ? event.name : "<null>");
}

}
}
