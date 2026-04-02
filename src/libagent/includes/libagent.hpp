#pragma once

#include <JuiceAgent/Logger.hpp>
#include <jni.h>
#include <jvmti.h>

namespace JuiceAgent {
    class Agent {
        private:
            JavaVM* jvm;
            jvmtiEnv* jvmti;
    
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

            // setters
            void set_jvm(JavaVM* jvm) { this->jvm = jvm; }
            void set_jvmti(jvmtiEnv* jvmti) { this->jvmti = jvmti; }
    };
}