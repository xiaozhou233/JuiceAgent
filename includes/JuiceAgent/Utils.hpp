#pragma once

#include <jvm/jni.h>
#include <jvm/jvmti.h>
#include <JuiceAgent/Logger.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <charconv>
#include <string_view>
#include <system_error>

namespace JuiceAgent::Utils
{
    // JNIEnv::FindClass expects the internal form of a class name where the
    // package separator is '/' (e.g. "Example/Main"). Configs and docs use the
    // Java binary name with '.' (e.g. "Example.Main"), so normalize it here to
    // keep both forms working.
    inline std::string to_jni_class_name(const char* name)
    {
        std::string result(name ? name : "");

        for (char& c : result) {
            if (c == '.') {
                c = '/';
            }
        }

        return result;
    }

    inline bool check_and_clear_exception(JNIEnv* env, const char* context)
    {
        if (!env->ExceptionCheck()) {
            return false;
        }

        spdlog::error("JNI Exception occurred at: {}", context);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return true;
    }

    // Call a static Java method with a single string parameter
    inline bool call_static_java_method(JNIEnv* env, const char* clazz, const char* method, const char* params)
    {
        if (!env || !clazz || !method) {
            spdlog::error("Invalid JNI arguments");
            return false;
        }

        constexpr const char* signature = "(Ljava/lang/String;)V";

        spdlog::debug("Calling Java method: {}.{} {}", clazz, method, signature);

        std::string jni_class = to_jni_class_name(clazz);
        jclass cls = env->FindClass(jni_class.c_str());
        if (!cls) {
            check_and_clear_exception(env, "FindClass failed");
            spdlog::error("Class not found: {}", clazz);
            return false;
        }

        jmethodID mid = env->GetStaticMethodID(cls, method, signature);
        jstring j_params = mid
            ? env->NewStringUTF(params ? params : "")
            : nullptr;

        if (!mid) {
            check_and_clear_exception(env, "GetStaticMethodID failed");
            spdlog::error("Method not found: {}", method);
        }
        else if (!j_params) {
            spdlog::error("Failed to create jstring");
        }

        if (!mid || !j_params) {
            if (j_params) {
                env->DeleteLocalRef(j_params);
            }
            env->DeleteLocalRef(cls);
            return false;
        }

        env->CallStaticVoidMethod(cls, mid, j_params);

        bool success = !env->ExceptionCheck();

        check_and_clear_exception(env, "CallStaticVoidMethod failed");

        env->DeleteLocalRef(j_params);
        env->DeleteLocalRef(cls);

        return success;
    }
    
    inline bool call_static_java_method(JNIEnv* env, const char* clazz, const char* method)
    {
        if (!env || !clazz || !method) {
            spdlog::error("Invalid JNI arguments");
            return false;
        }

        constexpr const char* signature = "()V";

        spdlog::debug("Calling Java method: {}.{} {}", clazz, method, signature);

        std::string jni_class = to_jni_class_name(clazz);
        jclass cls = env->FindClass(jni_class.c_str());
        if (!cls) {
            check_and_clear_exception(env, "FindClass failed");
            spdlog::error("Class not found: {}", clazz);
            return false;
        }

        jmethodID mid = env->GetStaticMethodID(cls, method, signature);
        if (!mid) {
            check_and_clear_exception(env, "GetStaticMethodID failed");
            spdlog::error("Method not found: {}", method);
            env->DeleteLocalRef(cls);
            return false;
        }

        env->CallStaticVoidMethod(cls, mid);

        bool success = !env->ExceptionCheck();
        check_and_clear_exception(env, "CallStaticVoidMethod failed");

        env->DeleteLocalRef(cls);

        return success;
    }
    
    class Serializer {
    public:
        template<typename T>
        Serializer& add_kv(std::string_view key, const T& value)
        {
            append_escaped(key);
            s += '=';

            using ValueType = std::decay_t<T>;

            if constexpr (std::is_same_v<ValueType, bool>) {
                s += value ? "true" : "false";
            }
            else if constexpr (std::is_integral_v<ValueType>) {
                append_number(value);
            }
            else if constexpr (std::is_floating_point_v<ValueType>) {
                append_number(value);
            }
            else {
                append_escaped(std::string_view(value));
            }

            s += ';';
            return *this;
        }

        void clear() noexcept
        {
            s.clear();
        }

        bool empty() const noexcept
        {
            return s.empty();
        }

        const std::string& serialize() const noexcept
        {
            return s;
        }

    private:
        void append_escaped(std::string_view input)
        {
            for (char c : input) {
                if (c == '=' || c == ';' || c == '\\') {
                    s += '\\';
                }

                s += c;
            }
        }

        template<typename T>
        void append_number(T value)
        {
            char buffer[64];

            auto [end, ec] = std::to_chars(
                buffer,
                buffer + sizeof(buffer),
                value
            );

            if (ec == std::errc{}) {
                s.append(buffer, end);
            }
        }

        std::string s;
    };

    inline jobjectArray to_java_string_array(
        JNIEnv* env,
        const std::vector<std::string>& values)
    {

        if (!env) {
            return nullptr;
        }

        jclass string_class = env->FindClass("java/lang/String");
        if (!string_class) {
            return nullptr;
        }

        jobjectArray result = env->NewObjectArray(
            static_cast<jsize>(values.size()),
            string_class,
            nullptr
        );

        if (!result) {
            env->DeleteLocalRef(string_class);
            return nullptr;
        }

        for (jsize i = 0; i < static_cast<jsize>(values.size()); ++i) {
            jstring value = env->NewStringUTF(values[i].c_str());

            if (!value) {
                env->DeleteLocalRef(result);
                env->DeleteLocalRef(string_class);
                return nullptr;
            }

            env->SetObjectArrayElement(result, i, value);
            env->DeleteLocalRef(value);

            if (env->ExceptionCheck()) {
                env->DeleteLocalRef(result);
                env->DeleteLocalRef(string_class);
                return nullptr;
            }
        }

        env->DeleteLocalRef(string_class);
        return result;
    }

    inline std::string toDelimitedString(const std::vector<std::string>& values,
                    const std::string& delimiter = ",")
    {
        std::string result;

        for (const auto& value : values) {
            if (!result.empty()) {
                result += delimiter;
            }
            result += value;
        }

        return result;
    }


    class File {
    public:
        static std::string write_to_tempfile(
            const unsigned char* data,
            std::size_t size,
            const std::string& name
        )
        {
#ifdef _WIN32
            const char* env = std::getenv("TEMP");
#else
            const char* env = std::getenv("TMPDIR");
#endif

            std::filesystem::path path = std::filesystem::path(env ? env : ".") / name;

            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out) {
                spdlog::error("Failed to create temp file {}", path.string());
                return {};
            }

            out.write(reinterpret_cast<const char*>(data), size);
            return path.string();
        }
    };
} // namespace JuiceAgent::Utils
