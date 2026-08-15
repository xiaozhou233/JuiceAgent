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

namespace JuiceAgent::Utils
{
    template<typename T>
    class LocalRef {
    public:
        LocalRef(JNIEnv* env, T ref = nullptr)
            : _env(env), _ref(ref) {}

        ~LocalRef()
        {
            reset();
        }

        LocalRef(const LocalRef&) = delete;
        LocalRef& operator=(const LocalRef&) = delete;

        LocalRef(LocalRef&& other) noexcept
            : _env(other._env), _ref(other._ref)
        {
            other._ref = nullptr;
        }

        T get() const
        {
            return _ref;
        }

        operator T() const
        {
            return _ref;
        }

        void reset(T ref = nullptr)
        {
            if (_ref) {
                _env->DeleteLocalRef(_ref);
            }
            _ref = ref;
        }

    private:
        JNIEnv* _env;
        T _ref;
    };

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

    inline bool call_java_impl(JNIEnv* env, const char* clazz, const char* method, const char* params)
    {
        if (!env || !clazz || !method) {
            spdlog::error("Invalid JNI arguments");
            return false;
        }

        constexpr const char* signature = "(Ljava/lang/String;)V";

        spdlog::debug("Calling Java method: {}.{} {}", clazz, method, signature);

        jclass cls = env->FindClass(clazz);
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

    class Serializer {
    public:
        template<typename T>
        Serializer& add_kv(const std::string& key, const T& value)
        {
            s += escape(key);
            s += '=';

            if constexpr (std::is_same_v<T, bool>) {
                s += value ? "true" : "false";
            }
            else if constexpr (std::is_arithmetic_v<T>) {
                s += escape(std::to_string(value));
            }
            else {
                s += escape(value);
            }

            s += ';';
            return *this;
        }

        void clear()
        {
            s.clear();
        }

        bool empty() const
        {
            return s.empty();
        }

        const std::string& serialize() const
        {
            return s;
        }

    private:
        static std::string escape(const std::string& input)
        {
            std::string out;
            out.reserve(input.size());

            for (char c : input) {
                if (c == '=' || c == ';' || c == '\\') {
                    out += '\\';
                }
                out += c;
            }

            return out;
        }

        std::string s;
    };

    class Deserializer {
    public:
        Deserializer() = default;

        explicit Deserializer(const std::string& input)
        {
            parse(input);
        }

        void parse(const std::string& input)
        {
            data.clear();

            std::string key;
            std::string value;
            bool reading_key = true;
            bool escaped = false;

            auto commit = [&]() {
                if (!key.empty()) {
                    data[unescape(key)] = unescape(value);
                }
                key.clear();
                value.clear();
                reading_key = true;
            };

            for (char c : input) {
                if (escaped) {
                    (reading_key ? key : value) += c;
                    escaped = false;
                }
                else if (c == '\\') {
                    escaped = true;
                }
                else if (reading_key && c == '=') {
                    reading_key = false;
                }
                else if (c == ';') {
                    commit();
                }
                else {
                    (reading_key ? key : value) += c;
                }
            }

            commit();
        }

        bool has(const std::string& key) const
        {
            return data.find(key) != data.end();
        }

        template<typename T>
        T get(const std::string& key, T def) const
        {
            auto it = data.find(key);
            if (it == data.end()) {
                return def;
            }

            return convert<T>(it->second, def);
        }

        void clear()
        {
            data.clear();
        }

        bool empty() const
        {
            return data.empty();
        }

        std::size_t size() const
        {
            return data.size();
        }

    private:
        static std::string unescape(const std::string& input)
        {
            std::string out;
            out.reserve(input.size());

            bool escaped = false;

            for (char c : input) {
                if (escaped) {
                    out += c;
                    escaped = false;
                }
                else if (c == '\\') {
                    escaped = true;
                }
                else {
                    out += c;
                }
            }

            return out;
        }

        static bool to_bool(const std::string& v, bool def)
        {
            if (v == "true" || v == "1") {
                return true;
            }
            if (v == "false" || v == "0") {
                return false;
            }
            return def;
        }

        template<typename T>
        static T convert(const std::string& v, T def)
        {
            if constexpr (std::is_same_v<T, std::string>) {
                return v;
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return to_bool(v, def);
            }
            else if constexpr (std::is_integral_v<T>) {
                try {
                    return static_cast<T>(std::stoll(v));
                }
                catch (...) {
                    return def;
                }
            }
            else if constexpr (std::is_floating_point_v<T>) {
                try {
                    return static_cast<T>(std::stold(v));
                }
                catch (...) {
                    return def;
                }
            }
            else {
                return def;
            }
        }

        std::unordered_map<std::string, std::string> data;
    };

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
