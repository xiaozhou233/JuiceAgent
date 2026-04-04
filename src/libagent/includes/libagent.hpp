#pragma once

#include <JuiceAgent/Logger.hpp>
#include <jni.h>
#include <jvmti.h>
#include <eventbus.hpp>

namespace JuiceAgent {
    class Agent {
        private:
            JavaVM* jvm;
            jvmtiEnv* jvmti;
            EventBus eventbus;
    
        private:
            Agent() = default;
            ~Agent() = default;
            
        public:
            // ===== Singleton =====
            static Agent& instance();

            Agent(const Agent&) = delete;
            Agent& operator=(const Agent&) = delete;

            // EntryPoint
            bool init(JavaVM* jvm, JNIEnv* env, jvmtiEnv* jvmti);

            // getters
            JavaVM* get_jvm() const { return jvm; }
            jvmtiEnv* get_jvmti() const { return jvmti; }
            EventBus& get_eventbus() { return eventbus; }

            // setters
            void set_jvm(JavaVM* jvm) { this->jvm = jvm; }
            void set_jvmti(jvmtiEnv* jvmti) { this->jvmti = jvmti; }
    };
}