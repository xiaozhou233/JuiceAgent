#pragma once

#include <jni.h>
#include <jvmti.h>
#include <JuiceAgent/Logger.hpp>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <stdexcept>

// Check for JNI exceptions and clear them
inline bool check_and_clear_exception(JNIEnv* env, const char* context) {
    if (env->ExceptionCheck()) {
        PLOGE << "JNI Exception occurred at: " << context;
        env->ExceptionDescribe();
        env->ExceptionClear();
        return true;
    }
    return false;
}

namespace JuiceAgent::Utils
{
    inline bool call_java_impl(JNIEnv* env, const char* clazz, const char* method, const char* params) {
        if (!env || !clazz || !method) {
            PLOGE << "Invalid JNI arguments";
            return false;
        }

        constexpr const char* method_signature = "(Ljava/lang/String;)V";

        PLOGD << "Calling Java method: " << clazz << "." << method << " " << method_signature;

        jclass cls = env->FindClass(clazz);
        if (!cls) {
            check_and_clear_exception(env, "FindClass failed");
            PLOGE << "Class not found: " << clazz;
            return false;
        }

        jmethodID mid = env->GetStaticMethodID(cls, method, method_signature);
        if (!mid) {
            check_and_clear_exception(env, "GetStaticMethodID failed");
            PLOGE << "Method not found: " << method;
            env->DeleteLocalRef(cls);
            return false;
        }

        jstring j_params = env->NewStringUTF(params ? params : "");
        if (!j_params) {
            PLOGE << "Failed to create jstring";
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
    private:
        std::string s;

    private:
        static std::string escape(const std::string& input) {
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

    public:
        Serializer() = default;

        Serializer& add_kv(const std::string& key, const std::string& value) {
            s += escape(key);
            s += '=';
            s += escape(value);
            s += ';';
            return *this;
        }

        Serializer& add_kv(const std::string& key, const char* value) {
            return add_kv(key, std::string(value));
        }

        Serializer& add_kv(const std::string& key, int value) {
            return add_kv(key, std::to_string(value));
        }

        Serializer& add_kv(const std::string& key, bool value) {
            return add_kv(key, value ? "true" : "false");
        }

        void clear() {
            s.clear();
        }

        bool empty() const {
            return s.empty();
        }

        const std::string& serialize() const {
            return s;
        }
    };


    class Deserializer {
    private:
        std::unordered_map<std::string, std::string> data;

    private:
        static std::string unescape(const std::string& input) {
            std::string out;
            out.reserve(input.size());

            bool escaped = false;

            for (char c : input) {
                if (escaped) {
                    out += c;
                    escaped = false;
                    continue;
                }

                if (c == '\\') {
                    escaped = true;
                    continue;
                }

                out += c;
            }

            return out;
        }

        static bool to_bool(const std::string& v, bool def) {
            if (v == "true" || v == "1") return true;
            if (v == "false" || v == "0") return false;
            return def;
        }

        template<typename T>
        static T convert(const std::string& v, T def) {
            if constexpr (std::is_same_v<T, std::string>) {
                return v;
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return to_bool(v, def);
            }
            else if constexpr (std::is_integral_v<T>) {
                try {
                    long long n = std::stoll(v);
                    return static_cast<T>(n);
                } catch (...) {
                    return def;
                }
            }
            else if constexpr (std::is_floating_point_v<T>) {
                try {
                    long double n = std::stold(v);
                    return static_cast<T>(n);
                } catch (...) {
                    return def;
                }
            }
            else {
                return def;
            }
        }

    public:
        Deserializer() = default;

        explicit Deserializer(const std::string& input) {
            parse(input);
        }

        void parse(const std::string& input) {
            data.clear();

            std::string key;
            std::string value;

            bool reading_key = true;
            bool escaped = false;

            for (char c : input) {
                if (escaped) {
                    (reading_key ? key : value) += c;
                    escaped = false;
                    continue;
                }

                if (c == '\\') {
                    escaped = true;
                    continue;
                }

                if (reading_key && c == '=') {
                    reading_key = false;
                    continue;
                }

                if (c == ';') {
                    if (!key.empty()) {
                        data[unescape(key)] = unescape(value);
                    }

                    key.clear();
                    value.clear();
                    reading_key = true;
                    continue;
                }

                (reading_key ? key : value) += c;
            }

            if (!key.empty()) {
                data[unescape(key)] = unescape(value);
            }
        }

        bool has(const std::string& key) const {
            return data.find(key) != data.end();
        }

        template<typename T>
        T get(const std::string& key, T def) const {
            auto it = data.find(key);
            if (it == data.end()) {
                return def;
            }

            return convert<T>(it->second, def);
        }

        void clear() {
            data.clear();
        }

        bool empty() const {
            return data.empty();
        }

        std::size_t size() const {
            return data.size();
        }
    };

    class File {
    public:
        static inline std::string make_timestamp_filename(const std::string& name) {
            using namespace std::chrono;

            auto ms = duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()
            ).count();

            return std::to_string(ms) + "_" + name;
        }

        static inline std::string write_to_tempfile(
            const unsigned char* data,
            std::size_t size,
            const std::string& name
        ) {
    #ifdef _WIN32
            const char* env = std::getenv("TEMP");
            std::filesystem::path dir = env ? env : ".";
    #else
            const char* env = std::getenv("TMPDIR");
            std::filesystem::path dir = env ? env : "/tmp";
    #endif

            auto path = dir / make_timestamp_filename(name);

            std::ofstream out(path, std::ios::binary);
            if (!out)
                throw std::runtime_error("Failed to create temp file");

            out.write(reinterpret_cast<const char*>(data), size);
            return path.string();
        }
    };
} // namespace JuiceAgent::Utils

