#pragma once

#include <JuiceAgent/Logger.hpp>
#include <jni.h>
#include <jvmti.h>
#include <event/eventbus.hpp>
#include <JuiceAgent/Config.hpp>
#include <modules/ModuleManager.hpp>

namespace JuiceAgent {
    class Agent {
        private:
            JavaVM* jvm;
            jvmtiEnv* jvmti;
            JNIEnv* env;
            EventBus eventbus;
            JuiceAgent::Config::Config config;
            JuiceAgent::Core::Modules::ModuleManager module_manager;
    
        private:
            Agent() = default;
            ~Agent() = default;
            
        public:
            // ===== Singleton =====
            static Agent& instance();

            Agent(const Agent&) = delete;
            Agent& operator=(const Agent&) = delete;

            // EntryPoint
            bool init(JavaVM* jvm, JNIEnv* env, jvmtiEnv* jvmti, std::string& runtime_dir);

            // getters
            JavaVM* get_jvm() const { return jvm; }
            jvmtiEnv* get_jvmti() const { return jvmti; }
            JNIEnv* get_env() const { return env; }
            EventBus& get_eventbus() { return eventbus; }
            JuiceAgent::Config::Config& get_config() { return config; }

            // setters
            void set_jvm(JavaVM* jvm) { this->jvm = jvm; }
            void set_jvmti(jvmtiEnv* jvmti) { this->jvmti = jvmti; }
            void set_env(JNIEnv* env) { this->env = env; }
            void set_config(JuiceAgent::Config::Config& config) { this->config = config; }
    };
}