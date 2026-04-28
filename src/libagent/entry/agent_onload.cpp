#include <jni.h>
#include <jvmti.h>
#include <filesystem>
#include <string>
#include <jni_impl.hpp>
#include <JuiceAgent/JuiceAgent-API-1.4.1+build.1.hpp>

// Global state
static JavaVM* g_vm = nullptr;
static jvmtiEnv* g_jvmti = nullptr;

// Forward declaration
void JNICALL OnVMInit(jvmtiEnv* jvmti, JNIEnv* env, jthread thread);

// Utility: check JVMTI error
static bool CheckJvmti(jvmtiError err, const char* action) {
    if (err != JVMTI_ERROR_NONE) {
        PLOGE << action << " failed, code=" << err;
        return false;
    }
    return true;
}

// VMInit callback
void JNICALL OnVMInit(jvmtiEnv* jvmti, JNIEnv* env, jthread thread) {
    PLOGI << "JVMTI VMInit triggered";

    std::string runtime_dir = std::filesystem::current_path().string();

    if (runtime_dir.empty()) {
        PLOGE << "runtime_dir is empty";
        return;
    }

    PLOGI << "Runtime directory: " << runtime_dir;

    // Inject JuiceAgent-API
        // Save bytes to file
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
        
    PLOGD << "Saved JuiceAgent-API to: " << juiceagent_api_path;
    PLOGD << "JuiceAgent-API size: " << JuiceAgent::Resource::juiceagent_api_bytes_len;
    PLOGD << "JuiceAgent-API SHA256: " << JuiceAgent::Resource::juiceagent_api_bytes_sha256;
    
    // Inject JuiceAgent jar into system class loader
    jint status = jvmti->AddToSystemClassLoaderSearch(juiceagent_api_path.c_str());
    if (status != JNI_OK) {
        PLOGE << "AddToSystemClassLoaderSearch failed: " << status;
        return;
    }

    PLOGI << "Jar injected successfully: " << juiceagent_api_path;

    JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();
    agent.init(g_vm, env, g_jvmti, runtime_dir);

    PLOGI << "JuiceAgent initialized successfully";
}

// Agent entry
JNIEXPORT jint JNICALL
Agent_OnLoad(JavaVM* vm, char* options, void* reserved) {
    InitLogger();

    PLOGI << "Agent_OnLoad";

    g_vm = vm;

    // Get JVMTI first
    jint result = vm->GetEnv(reinterpret_cast<void**>(&g_jvmti), JVMTI_VERSION_1_2);
    if (result != JNI_OK || g_jvmti == nullptr) {
        PLOGE << "Failed to get JVMTI, code=" << result;
        return JNI_ERR;
    }

    // Optional capabilities
    jvmtiCapabilities caps{};
    CheckJvmti(g_jvmti->AddCapabilities(&caps), "AddCapabilities");

    // Register callbacks
    jvmtiEventCallbacks callbacks{};
    callbacks.VMInit = &OnVMInit;

    if (!CheckJvmti(
            g_jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks)),
            "SetEventCallbacks")) {
        return JNI_ERR;
    }

    if (!CheckJvmti(
            g_jvmti->SetEventNotificationMode(
                JVMTI_ENABLE,
                JVMTI_EVENT_VM_INIT,
                nullptr),
            "Enable VM_INIT")) {
        return JNI_ERR;
    }

    PLOGI << "Agent loaded, waiting for VMInit...";
    return JNI_OK;
}

// Optional unload
JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM* vm) {
    PLOGI << "Agent_OnUnload";
}