#include <string>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent.hpp>
#include <JuiceAgent/Config.hpp>
#include <jni_common.hpp>
#include <services.hpp>

namespace JuiceAgent {
    // Singleton
    Agent& Agent::getInstance() {
        static Agent instance;
        return instance;
    }

    bool Agent::preload(JavaVM* jvm, JNIEnv* env, jvmtiEnv* jvmti, std::string& runtime_dir) {
        // Check if the agent is already loaded
        if(Agent::getInstance().isLoaded()) {
            spdlog::warn("[JuiceAgent] Agent already loaded! Loading again will cause problems!");
        }

        // Environment check
        if(!jvm || jvm==nullptr) {
            spdlog::error("[JuiceAgent] No JVM found!");
            return false;
        }
        if(!env || env==nullptr) {
            spdlog::error("[JuiceAgent] Could not get JNIEnv!");
            return false;
        }
        if(!jvmti || jvmti==nullptr) {
            spdlog::error("[JuiceAgent] Could not get JVMTI!");
            return false;
        }
        if(runtime_dir.empty()) {
            spdlog::warn("[JuiceAgent] No runtime directory specified!");
            spdlog::warn("[JuiceAgent] No config provided, Modules will not be loaded!");
        } else {
            JuiceAgent::Config::Config cfg(runtime_dir);
            setConfig(cfg);
            if(!getConfig().is_valid()) {
                spdlog::warn("[JuiceAgent] Config is not valid!");
                spdlog::warn("[JuiceAgent] No config provided, Modules will not be loaded!");
            }
        }
        

        // Set Environment
        setJavaVM(jvm);
        setJNIEnv(env);
        setJVMTI(jvmti);
        // Debug
        spdlog::debug("[JuiceAgent] JavaVM: {:p}, JNIEnv: {:p}, JVMTI: {:p}",
              (void*)jvm,
              (void*)env,
              (void*)jvmti);
    
        setLoaded(true);

        return true;
    }

    bool Agent::init() {
        if(!isLoaded()) {
            spdlog::error("[JuiceAgent] Agent not loaded! Please call preload() first!");
            return false;
        }
        
        // Add Abilities
        jvmtiCapabilities caps{};
        caps.can_get_bytecodes = 1;
        caps.can_redefine_classes = 1;
        caps.can_redefine_any_class = 1;
        caps.can_retransform_classes = 1;
        caps.can_retransform_any_class = 1;    
        caps.can_generate_all_class_hook_events = 1;
        jint result = getJVMTI()->AddCapabilities(&caps);
        if(result != JVMTI_ERROR_NONE) {
            spdlog::error("[JuiceAgent] Could not add JVMTI capabilities: {}", result);
            return false;
        }
        spdlog::info("[JuiceAgent] Added JVMTI capabilities");

        // Register callbacks
        jvmtiEventCallbacks callbacks{};
        callbacks.ClassFileLoadHook = &ClassFileLoadHook;
        result = getJVMTI()->SetEventCallbacks(&callbacks, sizeof(callbacks));
        if(result != JVMTI_ERROR_NONE) {
            spdlog::error("[JuiceAgent] Could not set JVMTI event callbacks: {}", result);
        }

        // Enable events
        result = getJVMTI()->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);
        if(result != JVMTI_ERROR_NONE) {
            spdlog::error("[JuiceAgent] Could not enable JVMTI event notifications: {}", result);
        }

        // Initialize services (and their plugins)
        if(!JuiceAgent::Services::initializeAll()) {
            spdlog::error("[JuiceAgent] Failed to initialize services");
            return false;
        }

        return true;
    }
}