#include <services/base.hpp>
#include <services/service_manager.hpp>
#include <event/event_type.hpp>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/Utils.hpp>
#include <global.hpp>

#include <string_view>
#include <cstring>

namespace JuiceAgent {
namespace Services {

class JuiceRemapperService : public IService {
public:
    const char* name() const override { return "JuiceRemapperService"; }

    bool onInitialize() override {
        listen<JuiceRemapperService, EventLoaded>(
            EventId::Loaded,
            &JuiceRemapperService::onLoaded
        );

        listen<JuiceRemapperService, EventClassFileLoadHook>(
            EventId::ClassFileLoadHook,
            &JuiceRemapperService::onClassFileLoadHook
        );

        spdlog::debug("[JuiceRemapperService] Initialized");
        return true;
    }

    void onShutdown() override {
        unlistenAll();
        spdlog::debug("[JuiceRemapperService] Shutdown");
    }

private:
    void onLoaded(const EventLoaded& /*event*/) {
        auto& agent = Agent::getInstance();
        auto& cfg = agent.getConfig();

        const bool enabled = cfg.get<bool>(
            "JuiceAgent.Modules.JuiceRemapper.Enabled", false);

        if (!enabled) {
            spdlog::debug("[JuiceRemapperService] Disabled by config");
            return;
        }

        spdlog::debug("[JuiceRemapperService] Enabled. Waiting for addInclude to start remapping");
    }

    void onClassFileLoadHook(const EventClassFileLoadHook& e) {
        if (!e.name || !e.jni_env || !e.jvmti_env ||
            !e.classbytes || e.class_data_len <= 0) {
            return;
        }

        auto& remapper = JuiceAgent::Services::Remapper::getInstance();
        if (!remapper.isInit())
            return;

        if (std::string_view(e.name).starts_with("cn/xiaozhou233/juiceremapper/"))
            return;

        // Include/Exclude filter check
        if (!remapper.shouldRemap(e.name)) {
            spdlog::trace("[JuiceRemapperService] Skipped by filter: {}", e.name);
            return;
        }

        jclass remapClass = remapper.getRemapClass();
        jmethodID remapMethod = remapper.getRemapMethodId();
        if (!remapClass || !remapMethod) {
            spdlog::error("[JuiceRemapperService] Remap declarations not cached");
            return;
        }

        JNIEnv* env = e.jni_env;

        // className: const char* -> jstring
        jstring jClassName = env->NewStringUTF(e.name);
        if (!jClassName) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            spdlog::error("[JuiceRemapperService] Failed to create className jstring");
            return;
        }

        // classBytes: const unsigned char* -> jbyteArray
        jbyteArray jClassBytes = env->NewByteArray(e.class_data_len);
        if (!jClassBytes) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            env->DeleteLocalRef(jClassName);
            spdlog::error("[JuiceRemapperService] Failed to create classBytes jbyteArray");
            return;
        }
        env->SetByteArrayRegion(
            jClassBytes, 0, e.class_data_len,
            reinterpret_cast<const jbyte*>(e.classbytes));
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            env->DeleteLocalRef(jClassName);
            env->DeleteLocalRef(jClassBytes);
            spdlog::error("[JuiceRemapperService] Failed to fill classBytes");
            return;
        }

        // Call static byte[] Remap.remap(String, byte[])
        jbyteArray jResult = static_cast<jbyteArray>(
            env->CallStaticObjectMethod(remapClass, remapMethod, jClassName, jClassBytes));
        if (env->ExceptionCheck() || !jResult) {
            env->ExceptionClear();
            env->DeleteLocalRef(jClassName);
            env->DeleteLocalRef(jClassBytes);
            spdlog::warn("[JuiceRemapperService] Remap returned null/exception for: {}", e.name);
            return;
        }

        const jsize newLen = env->GetArrayLength(jResult);
        jbyte* newBytes = env->GetByteArrayElements(jResult, nullptr);
        if (!newBytes) {
            env->DeleteLocalRef(jClassName);
            env->DeleteLocalRef(jClassBytes);
            env->DeleteLocalRef(jResult);
            return;
        }

        // Write remapped bytes back to the JVMTI callback
        unsigned char* newBuf = nullptr;
        if (e.jvmti_env->Allocate(newLen, &newBuf) == JVMTI_ERROR_NONE) {
            std::memcpy(newBuf, newBytes, newLen);
            *e.new_class_data_len = newLen;
            *e.new_classbytes = newBuf;
            spdlog::info("[JuiceRemapperService] Remapped: {} ({} -> {} bytes)",
                         e.name, e.class_data_len, newLen);
        } else {
            spdlog::error("[JuiceRemapperService] Failed to allocate buffer for: {}", e.name);
        }

        env->ReleaseByteArrayElements(jResult, newBytes, JNI_ABORT);
        env->DeleteLocalRef(jClassName);
        env->DeleteLocalRef(jClassBytes);
        env->DeleteLocalRef(jResult);
    }
};

}
}

JUICEAGENT_REGISTER_SERVICE(::JuiceAgent::Services::JuiceRemapperService)
