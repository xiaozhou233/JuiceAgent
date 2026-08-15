#pragma once

#include <thread>
#include <chrono>

#include <JuiceAgent/Logger.hpp>
#include <jvm/jni.h>
#include <jvm/jvmti.h>

namespace JuiceAgent::Loader {

class Jvm {
public:
    explicit Jvm(int max_try = 30, int retry_delay_ms = 1000)
        : max_try_(max_try), retry_delay_ms_(retry_delay_ms) {}

    JavaVM* get_jvm() const { return jvm_; }
    JNIEnv* get_env() const { return env_; }
    jvmtiEnv* get_jvmti() const { return jvmti_; }

    bool attach() {
        reset();

        return acquire_jvm() && acquire_env() && acquire_jvmti();
    }

    bool detach() {
        if (jvm_ && attached_) {
            jint res = jvm_->DetachCurrentThread();
            if (res != JNI_OK) {
                spdlog::debug("DetachCurrentThread failed: {}", res);
                return false;
            }

            spdlog::debug("Thread detached");
            attached_ = false;
            env_ = nullptr;
        }
        return true;
    }

private:
    static inline thread_local JNIEnv* env_ = nullptr;

    JavaVM* jvm_ = nullptr;
    jvmtiEnv* jvmti_ = nullptr;

    bool attached_ = false;
    int max_try_;
    int retry_delay_ms_;

    void reset() {
        jvm_ = nullptr;
        jvmti_ = nullptr;
        env_ = nullptr;
        attached_ = false;
    }

    void retry(const char* what, int attempt) {
        spdlog::debug("Failed to get {}, attempt {}/{}", what, attempt, max_try_);
        std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms_));
    }

    template<typename Fn>
    bool acquire(const char* what, Fn&& attempt) {
        for (int i = 1; i <= max_try_; i++) {
            if (attempt()) {
                return true;
            }
            retry(what, i);
        }
        return false;
    }

    bool acquire_jvm() {
        return acquire("JavaVM", [this] {
            jvm_ = nullptr;
            return JNI_GetCreatedJavaVMs(&jvm_, 1, nullptr) == JNI_OK && jvm_ != nullptr;
        });
    }

    bool acquire_env() {
        return acquire("JNIEnv", [this] {
            env_ = nullptr;
            if (jvm_->GetEnv(reinterpret_cast<void**>(&env_), JNI_VERSION_1_6) == JNI_OK && env_ != nullptr) {
                return true;
            }

            env_ = nullptr;
            if (jvm_->AttachCurrentThread(reinterpret_cast<void**>(&env_), nullptr) == JNI_OK && env_ != nullptr) {
                attached_ = true;
                return true;
            }

            env_ = nullptr;
            return false;
        });
    }

    bool acquire_jvmti() {
        return acquire("JVMTI", [this] {
            jvmti_ = nullptr;
            return jvm_->GetEnv(reinterpret_cast<void**>(&jvmti_), JVMTI_VERSION_1_2) == JNI_OK && jvmti_ != nullptr;
        });
    }
};

} // namespace JuiceAgent::Loader
