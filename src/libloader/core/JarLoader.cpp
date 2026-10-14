#include <Loader.hpp>
#include <jni.h>
#include <jvmti.h>
#include <string>

#include <JuiceAgent/Utils.hpp>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/JuiceAgent-API-1.4.1+build.1.hpp>

int JuiceAgent::Loader::load_builtin(jvmtiEnv* jvmti) {
    // Extract JuiceAgent API to temp file
    std::string hash =
        JuiceAgent::Resource::juiceagent_api_bytes_sha256;

    std::string file_name =
        hash.substr(0, 8) + "_" +
        JuiceAgent::Resource::juiceagent_api_bytes_name;

    std::string juiceagent_api_path =
        JuiceAgent::Utils::File::write_to_tempfile(
            JuiceAgent::Resource::juiceagent_api_bytes,
            JuiceAgent::Resource::juiceagent_api_bytes_len,
            file_name
        );
    
    // Debug
    spdlog::debug("JuiceAgent API path: {}", juiceagent_api_path);
    spdlog::debug("JuiceAgent API size: {}", JuiceAgent::Resource::juiceagent_api_bytes_len);
    spdlog::debug("JuiceAgent API sha256: {}", JuiceAgent::Resource::juiceagent_api_bytes_sha256);

    // Load JuiceAgent API
    jvmtiError status = jvmti->AddToSystemClassLoaderSearch(juiceagent_api_path.c_str());
    if (status != JVMTI_ERROR_NONE) {
        spdlog::error("Failed to add JuiceAgent API to system classloader! code: {}", static_cast<int>(status));
        return false;
    }

    spdlog::info("JuiceAgent API loaded to system classloader!");

    return true;
}

int JuiceAgent::Loader::load_custom(jvmtiEnv* jvmti, const char* jar_path) {
    spdlog::info("Loading custom jar: {}", jar_path);

    jvmtiError status = jvmti->AddToSystemClassLoaderSearch(jar_path);
    if (status != JVMTI_ERROR_NONE) {
        spdlog::error("Failed to add custom jar to system classloader! code: {}", static_cast<int>(status));
        return false;
    }

    spdlog::info("Custom jar loaded to system classloader!");

    return true;
}