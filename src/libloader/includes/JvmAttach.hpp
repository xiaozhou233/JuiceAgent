#pragma once

#include <chrono>
#include <thread>

#include <JuiceAgent/Logger.hpp>
#include <jvm/jni.h>
#include <jvm/jvmti.h>

namespace JuiceAgent::Loader {

// RAII wrapper: attaches to the running JVM (with retry) and detaches on destruction.
class JvmAttach {
public:
    explicit JvmAttach(int max_try = 30, int retry_delay_ms = 1000)
        : max_try_(max_try), retry_delay_ms_(retry_delay_ms) {}

    ~JvmAttach() { detach(); }

    JvmAttach(const JvmAttach&) = delete;
    JvmAttach& operator=(const JvmAttach&) = delete;

    JavaVM* get_jvm() const { return jvm_; }
    JNIEnv* get_env() const { return env_; }
    jvmtiEnv* get_jvmti() const { return jvmti_; }

    bool attach() {
        jvm_ = nullptr;
        env_ = nullptr;
        jvmti_ = nullptr;
        attached_ = false;

        return retry("JavaVM", [this] {
            return JNI_GetCreatedJavaVMs(&jvm_, 1, nullptr) == JNI_OK && jvm_;
        }) && retry("JNIEnv", [this] {
            if (jvm_->GetEnv(reinterpret_cast<void**>(&env_), JNI_VERSION_1_8) == JNI_OK && env_)
                return true;
            env_ = nullptr;
            if (jvm_->AttachCurrentThread(reinterpret_cast<void**>(&env_), nullptr) == JNI_OK && env_) {
                attached_ = true;
                return true;
            }
            return false;
        }) && retry("JVMTI", [this] {
            return jvm_->GetEnv(reinterpret_cast<void**>(&jvmti_), JVMTI_VERSION_1_2) == JNI_OK && jvmti_;
        });
    }

    bool detach() noexcept {
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
    template<typename Fn>
    bool retry(const char* what, Fn&& attempt) {
        for (int i = 1; i <= max_try_; i++) {
            if (attempt())
                return true;
            spdlog::debug("Failed to get {}, attempt {}/{}", what, i, max_try_);
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms_));
        }
        return false;
    }

    JavaVM* jvm_ = nullptr;
    JNIEnv* env_ = nullptr;
    jvmtiEnv* jvmti_ = nullptr;
    bool attached_ = false;
    int max_try_;
    int retry_delay_ms_;
};

} // namespace JuiceAgent::Loader
