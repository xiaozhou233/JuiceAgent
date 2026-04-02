#include <jni_impl.hpp>
#include <algorithm>
#include <string>
#include <cstring>

// ============================
// Get class bytecode (already captured)
// ============================
JNIEXPORT jbyteArray JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytes
  (JNIEnv *env, jclass, jclass clazz) {

    auto& agent = JuiceAgent::Agent::instance();
    if (!check_env(agent)) return nullptr;

    char* signature = nullptr;
    if (agent.get_jvmti()->GetClassSignature(clazz, &signature, nullptr) != JVMTI_ERROR_NONE || !signature)
        return nullptr;

    // Convert Lxxx; -> xxx
    std::string internalName(signature + 1, strlen(signature) - 2);
    agent.get_jvmti()->Deallocate(reinterpret_cast<unsigned char*>(signature));

    // ============================
    // 1. Mark class for capture
    // ============================
    {
        std::lock_guard<std::mutex> lock(classDataMutex);
        classToCapture.insert(internalName);
    }

    // ============================
    // 2. Trigger class retransform
    // ============================
    jvmtiError err = agent.get_jvmti()->RetransformClasses(1, &clazz);
    if (err != JVMTI_ERROR_NONE) {
        PLOGE << "Retransform class failed: " << err;
        return nullptr;
    }

    // ============================
    // 3. Retrieve bytecode from cache
    // ============================
    ClassFileData data;
    {
        std::lock_guard<std::mutex> lock(classDataMutex);
        auto it = classFileDataMap.find(internalName);
        if (it != classFileDataMap.end()) {
            data = it->second;
        }
    }

    if (data.bytecode.empty()) {
        PLOGE << "Failed to capture bytecode for class: " << internalName;
        return nullptr;
    }

    jbyteArray out = env->NewByteArray(static_cast<jsize>(data.bytecode.size()));
    if (!out || env->ExceptionCheck()) {
        env->ExceptionClear();
        return nullptr;
    }

    env->SetByteArrayRegion(out, 0, static_cast<jsize>(data.bytecode.size()), reinterpret_cast<const jbyte*>(data.bytecode.data()));
    return out;
}

// ============================
// Get class bytecode by class name
// ============================
JNIEXPORT jbyteArray JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytesByName
  (JNIEnv *env, jclass loader_class, jstring name) {

    if (!name) return nullptr;
    auto& agent = JuiceAgent::Agent::instance();
    if (!check_env(agent)) return nullptr;

    // ============================
    // 1. Convert jstring -> internal name
    // ============================
    const char* utf = env->GetStringUTFChars(name, nullptr);
    if (!utf) return nullptr;
    std::string internalName(utf);
    env->ReleaseStringUTFChars(name, utf);

    // Convert dots to slashes: a.b.C -> a/b/C
    std::replace(internalName.begin(), internalName.end(), '.', '/');

    // ============================
    // 2. Iterate loaded classes
    // ============================
    jint count = 0;
    jclass* classes = nullptr;
    jvmtiEnv* jvmti = agent.get_jvmti();

    if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE || count == 0) {
        return nullptr;
    }

    jclass targetClass = nullptr;
    for (jint i = 0; i < count; i++) {
        char* signature = nullptr;
        if (jvmti->GetClassSignature(classes[i], &signature, nullptr) == JVMTI_ERROR_NONE && signature) {
            size_t len = strlen(signature);
            if (len > 2 && signature[0] == 'L' && signature[len - 1] == ';') {
                if (internalName == std::string_view(signature + 1, len - 2)) {
                    targetClass = classes[i];
                    jvmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
                    break;
                }
            }
            jvmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
        }
    }

    if (classes) {
        jvmti->Deallocate(reinterpret_cast<unsigned char*>(classes));
    }

    if (!targetClass) {
        PLOGE << "Class not loaded: " << internalName;
        return nullptr;
    }

    // ============================
    // 3. Reuse getClassBytes logic
    // ============================
    return Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytes(env, loader_class, targetClass);
}

// ============================
// Redefine class using jclass
// ============================
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClass
  (JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray class_bytes, jint class_bytes_len) {

    // Get singleton agent instance
    auto& agent = JuiceAgent::Agent::instance();
    if (!check_env(agent))
        return JNI_FALSE;

    // Get byte array elements
    jbyte* buf = env->GetByteArrayElements(class_bytes, nullptr);
    if (!buf) {
        PLOGE << "Cannot get class bytes";
        return JNI_FALSE;
    }

    // Prepare JVMTI class definition
    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = class_bytes_len;
    defs[0].class_bytes = reinterpret_cast<const unsigned char*>(buf);

    // Redefine class via JVMTI
    jvmtiError result = agent.get_jvmti()->RedefineClasses(1, defs);

    // Release byte array (no need to copy back)
    env->ReleaseByteArrayElements(class_bytes, buf, JNI_ABORT);

    if (result != JVMTI_ERROR_NONE) {
        PLOGE << "Redefine class failed: " << result;
        return JNI_FALSE;
    }

    PLOGD << "Class redefined successfully!";
    return JNI_TRUE;
}

// ============================================
// Redefine class using its fully qualified name
// Supports automatic '.' -> '/' conversion
// ============================================
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClassByName
  (JNIEnv *env, jclass loader_class, jstring class_name, jbyteArray class_bytes, jint class_bytes_len) {

    // Convert jstring to C string
    const char* cname = env->GetStringUTFChars(class_name, nullptr);
    if (!cname) {
        PLOGE << "Class name is NULL!";
        return JNI_FALSE;
    }

    // Copy and replace '.' with '/' for JVM internal class name
    std::string internal_name(cname);
    std::replace(internal_name.begin(), internal_name.end(), '.', '/');

    // Release the original Java string
    env->ReleaseStringUTFChars(class_name, cname);

    // Find class by internal name
    jclass clazz = env->FindClass(internal_name.c_str());
    if (!clazz) {
        PLOGE << "Cannot find class: " << internal_name;
        return JNI_FALSE;
    }

    // Call redefineClass with the found class
    return Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClass(
        env, loader_class, clazz, class_bytes, class_bytes_len
    );
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_retransformClass
  (JNIEnv* env, jclass, jclass clazz, jbyteArray bytecodes, jint length) {
    auto& agent = JuiceAgent::Agent::instance();

    // =========================
    // 1. Check environment & parameters
    // =========================
    if (!check_env(agent) || !clazz || !bytecodes) {
        return JNI_FALSE;
    }

    // =========================
    // 2. Get class internal name
    // =========================
    std::string nameDot = get_class_name(env, clazz);
    if (nameDot.empty()) {
        PLOGE << "Failed to get class name for retransform";
        return JNI_FALSE;
    }

    // Convert '.' to '/' for internal name
    for (auto& c : nameDot) if (c == '.') c = '/';

    // =========================
    // 3. Extract bytecode from jbyteArray
    // =========================
    jbyte* bytes = env->GetByteArrayElements(bytecodes, nullptr);
    jint len = env->GetArrayLength(bytecodes);
    if (!bytes || len <= 0) {
        return JNI_FALSE;
    }

    // =========================
    // 4. Cache bytecode & class info for ClassFileLoadHook
    // =========================
    {
        std::lock_guard<std::mutex> lock(classDataMutex);

        ClassFileData data;
        data.classname = nameDot;
        data.bytecode.assign(bytes, bytes + len);
        data.clazz = static_cast<jclass>(env->NewGlobalRef(clazz)); // prevent GC
        data.classloader = nullptr;                                  // optional
        data.protection_domain = nullptr;                            

        classFileDataMap[nameDot] = std::move(data);
    }

    env->ReleaseByteArrayElements(bytecodes, bytes, JNI_ABORT);

    // =========================
    // 5. Trigger JVMTI retransform
    // =========================
    jclass classes[1] = { clazz };
    jvmtiError err = agent.get_jvmti()->RetransformClasses(1, classes);

    if (err != JVMTI_ERROR_NONE) {
        PLOGE << "[retransformClass] failed: " << err << " for " << nameDot;
        return JNI_FALSE;
    }

    PLOGI << "[retransformClass] OK: " << nameDot << " (" << len << " bytes)";
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_retransformClassByName
  (JNIEnv* env, jclass, jstring className, jbyteArray bytecodes, jint length) {

    auto& agent = JuiceAgent::Agent::instance();

    if (!check_env(agent) || !className || !bytecodes) {
        return JNI_FALSE;
    }

    // =========================
    // 1. Get UTF string from jstring
    // =========================
    const char* utf = env->GetStringUTFChars(className, nullptr);
    if (!utf) return JNI_FALSE;

    std::string internalName(utf);
    env->ReleaseStringUTFChars(className, utf);

    // Convert '.' to '/' for internal name
    for (auto& c : internalName) if (c == '.') c = '/';

    // =========================
    // 2. Find loaded class by internal name (inline)
    // =========================
    jclass target = nullptr;
    {
        jint classCount = 0;
        jclass* loadedClasses = nullptr;

        if (agent.get_jvmti()->GetLoadedClasses(&classCount, &loadedClasses) == JVMTI_ERROR_NONE && classCount > 0) {
            for (jint i = 0; i < classCount; ++i) {
                char* sig = nullptr;
                if (agent.get_jvmti()->GetClassSignature(loadedClasses[i], &sig, nullptr) == JVMTI_ERROR_NONE && sig) {
                    // sig: "Lnet/minecraft/client/Minecraft;"
                    if (sig[0] == 'L') {
                        size_t len = strlen(sig);
                        if (len > 2 && internalName == std::string(sig + 1, len - 2)) {
                            target = loadedClasses[i];
                            agent.get_jvmti()->Deallocate(reinterpret_cast<unsigned char*>(sig));
                            break;
                        }
                    }
                    agent.get_jvmti()->Deallocate(reinterpret_cast<unsigned char*>(sig));
                }
            }
            agent.get_jvmti()->Deallocate(reinterpret_cast<unsigned char*>(loadedClasses));
        }
    }

    if (!target) {
        PLOGE << "[retransformClassByName] class not loaded: " << internalName;
        return JNI_FALSE;
    }

    // =========================
    // 3. Delegate to retransformClass
    // =========================
    return Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_retransformClass(
        env, nullptr, target, bytecodes, length
    );
}