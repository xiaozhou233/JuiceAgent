#pragma once

#include <JuiceAgent/Logger.hpp>
#include <jvm/jni.h>
#include <jvm/jvmti.h>
#include <thread>
#include <chrono>

namespace JuiceAgent::Loader::JvmManager
{
    class Jvm {
    private:
        JavaVM* jvm = nullptr;
        jvmtiEnv* jvmti = nullptr;

        thread_local static JNIEnv* env;

        bool attached = false;
        int max_try = 30;

    public:
        Jvm() = default;

        ~Jvm() = default;

        JavaVM* get_jvm() const { return jvm; }
        JNIEnv* get_env() const { return env; }
        jvmtiEnv* get_jvmti() const { return jvmti; }

        bool attach() {
            // reset state
            jvm = nullptr;
            jvmti = nullptr;
            env = nullptr;
            attached = false;

            jint status = JNI_ERR;

            // ======== Get JavaVM ========
            for (int i = 0; i < max_try; i++) {
                status = JNI_GetCreatedJavaVMs(&jvm, 1, nullptr);

                if (status == JNI_OK && jvm != nullptr) {
                    PLOGI << "Got JavaVM";
                    break;
                }

                PLOGE << "Failed to get JavaVM: " << status;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            if (!jvm) {
                PLOGF << "JavaVM not found!";
                return false;
            }

            // ======== Get JNIEnv ========
            for (int i = 0; i < max_try; i++) {
                status = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);

                if (status == JNI_OK && env != nullptr) {
                    PLOGI << "Got JNIEnv (already attached)";
                    break;
                }

                // try attach
                status = jvm->AttachCurrentThread((void**)&env, nullptr);
                if (status == JNI_OK && env != nullptr) {
                    attached = true;
                    PLOGI << "Attached current thread";
                    break;
                }

                PLOGE << "Attach failed: " << status;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            if (!env) {
                PLOGF << "Failed to get JNIEnv";
                return false;
            }

            // ======== Get JVMTI ========
            for (int i = 0; i < max_try; i++) {
                status = jvm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_2);

                if (status == JNI_OK && jvmti != nullptr) {
                    PLOGI << "Got JVMTI";
                    break;
                }

                PLOGE << "Failed to get JVMTI: " << status;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            if (!jvmti) {
                PLOGF << "Failed to get JVMTI";
                return false;
            }

            return true;
        }

        bool detach() {
            if (jvm && attached) {
                jint res = jvm->DetachCurrentThread();
                if (res != JNI_OK) {
                    PLOGE << "DetachCurrentThread failed: " << res;
                    return false;
                }

                PLOGI << "Thread detached";
                attached = false;
                env = nullptr;
            }
            return true;
        }
    };

    thread_local JNIEnv* Jvm::env = nullptr;

} // namespace JuiceAgent::Loader::JvmManager