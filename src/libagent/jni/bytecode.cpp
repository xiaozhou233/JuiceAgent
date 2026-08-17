#include <jni_common.hpp>
#include <global.hpp>
#include <algorithm>
#include <string>
#include <vector>
#include <cstring>

using JuiceAgent::Services::BytecodeStore;

namespace {

// Convert "Lcom/foo/Bar;" -> "com/foo/Bar"; returns empty on malformed input.
std::string signatureToInternal(const char* signature) {
    if (!signature) return {};
    size_t len = std::strlen(signature);
    if (len < 2 || signature[0] != 'L' || signature[len - 1] != ';') return {};
    return std::string(signature + 1, len - 2);
}

std::string dottedToInternal(std::string s) {
    std::replace(s.begin(), s.end(), '.', '/');
    return s;
}

// Find a loaded class by internal name ("com/foo/Bar"). Returns a local ref,
// or nullptr if not found. Caller must DeleteLocalRef when done.
jclass findLoadedClass(jvmtiEnv* jvmti, JNIEnv* env, const std::string& internalName) {
    jint count = 0;
    jclass* classes = nullptr;
    if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE || count == 0) {
        return nullptr;
    }

    jclass target = nullptr;
    for (jint i = 0; i < count; ++i) {
        char* sig = nullptr;
        if (jvmti->GetClassSignature(classes[i], &sig, nullptr) != JVMTI_ERROR_NONE || !sig) {
            continue;
        }
        std::string name = signatureToInternal(sig);
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(sig));
        if (name == internalName) {
            target = static_cast<jclass>(env->NewLocalRef(classes[i]));
            break;
        }
    }
    jvmti->Deallocate(reinterpret_cast<unsigned char*>(classes));
    return target;
}

}

// ---------------------------------------------------------------------------
// getClassBytes(Class) -> byte[]
// ---------------------------------------------------------------------------
JNIEXPORT jbyteArray JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytes
  (JNIEnv *env, jclass, jclass clazz) {

    auto& agent = JuiceAgent::Agent::getInstance();
    if (!check_env(agent) || !clazz) return nullptr;

    char* signature = nullptr;
    if (agent.getJVMTI()->GetClassSignature(clazz, &signature, nullptr) != JVMTI_ERROR_NONE || !signature) {
        return nullptr;
    }
    std::string internalName = signatureToInternal(signature);
    agent.getJVMTI()->Deallocate(reinterpret_cast<unsigned char*>(signature));
    if (internalName.empty()) return nullptr;

    BytecodeStore::getInstance().requestCapture(internalName);

    jvmtiError err = agent.getJVMTI()->RetransformClasses(1, &clazz);
    if (err != JVMTI_ERROR_NONE) {
        spdlog::error("[bytecode] RetransformClasses failed for {}: {}",
                      internalName, static_cast<int>(err));
        return nullptr;
    }

    auto captured = BytecodeStore::getInstance().takeCaptured(internalName);
    if (!captured || captured->bytecode.empty()) {
        spdlog::error("[bytecode] Failed to capture bytecode for class: {}", internalName);
        return nullptr;
    }

    jbyteArray out = env->NewByteArray(static_cast<jsize>(captured->bytecode.size()));
    if (!out || env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }
    env->SetByteArrayRegion(out, 0,
        static_cast<jsize>(captured->bytecode.size()),
        reinterpret_cast<const jbyte*>(captured->bytecode.data()));
    return out;
}

// ---------------------------------------------------------------------------
// getClassBytesByName(String) -> byte[]
// ---------------------------------------------------------------------------
JNIEXPORT jbyteArray JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytesByName
  (JNIEnv *env, jclass, jstring name) {

    if (!name) return nullptr;
    auto& agent = JuiceAgent::Agent::getInstance();
    if (!check_env(agent)) return nullptr;

    const char* utf = env->GetStringUTFChars(name, nullptr);
    if (!utf) return nullptr;
    std::string internalName = dottedToInternal(utf);
    env->ReleaseStringUTFChars(name, utf);

    jclass clazz = env->FindClass(internalName.c_str());
    if (!clazz) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        spdlog::error("[bytecode] Class not found: {}", internalName);
        return nullptr;
    }

    jbyteArray result = Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytes(env, nullptr, clazz);
    env->DeleteLocalRef(clazz);
    return result;
}

// ---------------------------------------------------------------------------
// redefineClass(Class, byte[], int) -> boolean
// ---------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClass
  (JNIEnv *env, jclass, jclass clazz, jbyteArray class_bytes, jint class_bytes_len) {

    auto& agent = JuiceAgent::Agent::getInstance();
    if (!check_env(agent) || !clazz || !class_bytes || class_bytes_len <= 0) {
        return JNI_FALSE;
    }

    // Validate the declared length against the actual array length to prevent
    // JVMTI from reading past the buffer.
    const jint array_len = env->GetArrayLength(class_bytes);
    if (class_bytes_len > array_len) {
        spdlog::error("[bytecode] class_bytes_len {} exceeds array length {}",
                      class_bytes_len, array_len);
        return JNI_FALSE;
    }

    jbyte* buf = env->GetByteArrayElements(class_bytes, nullptr);
    if (!buf) {
        spdlog::error("[bytecode] Cannot get class bytes");
        return JNI_FALSE;
    }

    jvmtiClassDefinition def{};
    def.klass = clazz;
    def.class_byte_count = class_bytes_len;
    def.class_bytes = reinterpret_cast<const unsigned char*>(buf);

    jvmtiError result = agent.getJVMTI()->RedefineClasses(1, &def);
    env->ReleaseByteArrayElements(class_bytes, buf, JNI_ABORT);

    if (result != JVMTI_ERROR_NONE) {
        spdlog::error("[bytecode] RedefineClasses failed: {}", static_cast<int>(result));
        return JNI_FALSE;
    }

    spdlog::debug("[bytecode] Class redefined successfully");
    return JNI_TRUE;
}

// ---------------------------------------------------------------------------
// redefineClassByName(String, byte[], int) -> boolean
// ---------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClassByName
  (JNIEnv *env, jclass, jstring class_name, jbyteArray class_bytes, jint class_bytes_len) {

    if (!class_name) return JNI_FALSE;
    auto& agent = JuiceAgent::Agent::getInstance();
    if (!check_env(agent)) return JNI_FALSE;

    const char* cname = env->GetStringUTFChars(class_name, nullptr);
    if (!cname) return JNI_FALSE;
    std::string internalName = dottedToInternal(cname);
    env->ReleaseStringUTFChars(class_name, cname);

    // JNI FindClass uses the caller's classloader, which is unreliable from a
    // native agent thread. Use JVMTI to search loaded classes instead.
    jclass clazz = findLoadedClass(agent.getJVMTI(), env, internalName);
    if (!clazz) {
        spdlog::error("[bytecode] Cannot find loaded class: {}", internalName);
        return JNI_FALSE;
    }

    jboolean result = Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClass(
        env, nullptr, clazz, class_bytes, class_bytes_len);
    env->DeleteLocalRef(clazz);
    return result;
}

// ---------------------------------------------------------------------------
// retransformClass(Class, byte[], int) -> boolean
//
// Queues new bytecode for the next ClassFileLoadHook (which will write it
// back via new_classbytes/new_class_data_len), then triggers RetransformClasses.
// The `length` parameter is kept for JNI signature compatibility but the
// actual length is read from the array.
// ---------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_retransformClass
  (JNIEnv* env, jclass, jclass clazz, jbyteArray bytecodes, jint /*length*/) {

    auto& agent = JuiceAgent::Agent::getInstance();
    if (!check_env(agent) || !clazz || !bytecodes) return JNI_FALSE;

    char* signature = nullptr;
    if (agent.getJVMTI()->GetClassSignature(clazz, &signature, nullptr) != JVMTI_ERROR_NONE || !signature) {
        return JNI_FALSE;
    }
    std::string internalName = signatureToInternal(signature);
    agent.getJVMTI()->Deallocate(reinterpret_cast<unsigned char*>(signature));
    if (internalName.empty()) return JNI_FALSE;

    jint len = env->GetArrayLength(bytecodes);
    if (len <= 0) return JNI_FALSE;

    jbyte* bytes = env->GetByteArrayElements(bytecodes, nullptr);
    if (!bytes) return JNI_FALSE;

    std::vector<unsigned char> newBytes(
        reinterpret_cast<const unsigned char*>(bytes),
        reinterpret_cast<const unsigned char*>(bytes) + len);
    env->ReleaseByteArrayElements(bytecodes, bytes, JNI_ABORT);

    BytecodeStore::getInstance().requestPatch(internalName, std::move(newBytes));

    jclass classes[1] = { clazz };
    jvmtiError err = agent.getJVMTI()->RetransformClasses(1, classes);
    if (err != JVMTI_ERROR_NONE) {
        spdlog::error("[bytecode] RetransformClasses failed for {}: {}",
                      internalName, static_cast<int>(err));
        return JNI_FALSE;
    }

    spdlog::info("[bytecode] Retransform queued: {} ({} bytes)", internalName, len);
    return JNI_TRUE;
}

// ---------------------------------------------------------------------------
// retransformClassByName(String, byte[], int) -> boolean
// ---------------------------------------------------------------------------
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_retransformClassByName
  (JNIEnv* env, jclass, jstring className, jbyteArray bytecodes, jint length) {

    if (!className) return JNI_FALSE;
    auto& agent = JuiceAgent::Agent::getInstance();
    if (!check_env(agent)) return JNI_FALSE;

    const char* utf = env->GetStringUTFChars(className, nullptr);
    if (!utf) return JNI_FALSE;
    std::string internalName = dottedToInternal(utf);
    env->ReleaseStringUTFChars(className, utf);

    jclass target = findLoadedClass(agent.getJVMTI(), env, internalName);
    if (!target) {
        spdlog::error("[bytecode] Class not loaded: {}", internalName);
        return JNI_FALSE;
    }

    jboolean result = Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_retransformClass(
        env, nullptr, target, bytecodes, length);
    env->DeleteLocalRef(target);
    return result;
}
