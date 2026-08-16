#pragma once
#include <jvm/jni.h>
#include <jvm/jvmti.h>

namespace JuiceAgent {
    class Agent{
        private:
            JavaVM* jvm;
            jvmtiEnv* jvmti;
            // Unsafe: JNIEnv
            JNIEnv* env;

            bool loaded = false;
        public:
            // Singleton
            static Agent& getInstance();

            Agent& operator=(const Agent&) = delete;

            // preload
            bool preload(JavaVM* jvm, JNIEnv* env, jvmtiEnv* jvmti, std::string& runtime_dir);
            // init
            bool init();
            
            // getters
            JavaVM* getJavaVM() const { return jvm; }
            jvmtiEnv* getJVMTI() const { return jvmti; }
            JNIEnv* getJNIEnv() const { return env; }
            bool isLoaded() const { return loaded; }

            // setters
            void setJavaVM(JavaVM* jvm) { this->jvm = jvm; }
            void setJVMTI(jvmtiEnv* jvmti) { this->jvmti = jvmti; }
            void setJNIEnv(JNIEnv* env) { this->env = env; }
            void setLoaded(bool loaded) { this->loaded = loaded; }

    };
}