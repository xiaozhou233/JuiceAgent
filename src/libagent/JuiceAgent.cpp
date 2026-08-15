#include <JuiceAgent.hpp>
#include <jni_impl.hpp>
#include <event/event_type.hpp>
#include <event/eventbus.hpp>
#include <JuiceAgent/Config.hpp>
#include <services.hpp>

namespace JuiceAgent {
    // ===== Singleton =====
    Agent& Agent::instance() {
        static Agent instance;
        return instance;
    }
    
    bool Agent::init(JavaVM* jvm, JNIEnv* env, jvmtiEnv* jvmti, std::string& runtime_dir) {
        try {
            spdlog::info("JuiceAgent Native EntryPoint Invoked!");

            // ======== neccessary environment check ========
            if (jvm == nullptr) {
                spdlog::error("Failed to initialize JuiceAgent: jvm is null");
                return false;
            }
            if (env == nullptr) {
                spdlog::error("Failed to initialize JuiceAgent: env is null");
                return false;
            }
            if (jvmti == nullptr) {
                spdlog::error("Failed to initialize JuiceAgent: jvmti is null");
                return false;
            }
            JuiceAgent::Config::Config cfg (runtime_dir);
            if (!cfg.is_valid()) {
                spdlog::error("Failed to initialize JuiceAgent: config is invalid");
                return false;
            }

            set_jvm(jvm);
            set_jvmti(jvmti);
            set_env(env);
            set_config(cfg);
            spdlog::debug("env: {}, jvmti: {}", fmt::ptr(env), fmt::ptr(jvmti));
            
            // Register events for services
            JuiceAgent::services::Manager::register_events();

            // PreLoad Event
            eventbus.post(EventPreLoad{
                .jvm = jvm,
                .env = env,
                .jvmti = jvmti
            });

            // ======== Application ability ========
            jvmtiCapabilities caps{};
            caps.can_get_bytecodes = 1;
            caps.can_redefine_classes = 1;
            caps.can_redefine_any_class = 1;
            caps.can_retransform_classes = 1;
            caps.can_retransform_any_class = 1;    
            caps.can_generate_all_class_hook_events = 1;
            jint result = jvmti->AddCapabilities(&caps);
            if (result != JNI_OK) {
                spdlog::error("Failed to add capabilities: {}", result);
                return false;
            }
            
            spdlog::info("Abilities added successfully");

            // ======== Register callbacks ========
            jvmtiEventCallbacks callbacks{};
            callbacks.ClassFileLoadHook = &ClassFileLoadHook;
            result = jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
            if (result != JNI_OK) {
                spdlog::error("Failed to set event callbacks: {}", result);
            }

            // Enable events
            result = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
            if (result != JNI_OK) {
                spdlog::error("Failed to enable event notification: {}", result);
            }

            // Loaded Event
            eventbus.post(EventLoaded{
                .jvm = jvm,
                .env = env,
                .jvmti = jvmti
            });
            return true;
        } catch (const std::exception& e) {
            spdlog::error("Exception: {}", e.what());
            return false;
        }
    }
}