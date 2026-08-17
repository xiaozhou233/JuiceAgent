#include <services/base.hpp>
#include <services/service_manager.hpp>
#include <event/event_type.hpp>
#include <JuiceAgent/Logger.hpp>
#include <global.hpp>

#include <cstring>

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
        if (!e.name) return;

        const std::size_t len = BytecodeStore::getInstance()
            .tryCapture(e.name, e.classbytes, e.class_data_len);

        if (len > 0) {
            spdlog::info("[BytecodeService] Captured class: {} (length: {})",
                         e.name, len);
        }
    }

    void patchBytecodes(const EventClassFileLoadHook& e) {
        if (!e.name || !e.jvmti_env ||
            !e.new_class_data_len || !e.new_classbytes) {
            return;
        }

        std::vector<unsigned char> bytes;
        if (!BytecodeStore::getInstance().takePatch(e.name, bytes) || bytes.empty()) {
            return;
        }

        const jlong new_len = static_cast<jlong>(bytes.size());
        unsigned char* new_buf = nullptr;
        if (e.jvmti_env->Allocate(new_len, &new_buf) != JVMTI_ERROR_NONE) {
            // Put bytes back so a future hook can retry instead of losing the patch.
            BytecodeStore::getInstance().putBackPatch(e.name, std::move(bytes));
            spdlog::error("[BytecodeService] Failed to allocate buffer for: {}", e.name);
            return;
        }

        std::memcpy(new_buf, bytes.data(), bytes.size());

        *e.new_class_data_len = static_cast<jint>(new_len);
        *e.new_classbytes = new_buf;

        spdlog::info("[BytecodeService] Patched class: {} (new length: {})",
                     e.name, new_len);
    }
};

}
}

JUICEAGENT_REGISTER_SERVICE(::JuiceAgent::Services::BytecodeService)
