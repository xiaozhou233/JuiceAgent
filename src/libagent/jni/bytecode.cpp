#include <jni_impl.hpp>
#include <algorithm>

JNIEXPORT jbyteArray JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytes
  (JNIEnv *env, jclass, jclass clazz) {
    auto& agent = JuiceAgent::Agent::instance();
    if (!check_env(agent))
        return nullptr;

    char* signature = nullptr;
    if (agent.get_jvmti()->GetClassSignature(clazz, &signature, nullptr) != JVMTI_ERROR_NONE || !signature) {
        return nullptr;
    }

    // Lxxx; -> xxx
    std::string internalName(signature + 1, strlen(signature) - 2);
    agent.get_jvmti()->Deallocate((unsigned char*)signature);

    classToCapture.insert(internalName); // Add to capture list

    jvmtiError err = agent.get_jvmti()->RetransformClasses(1, &clazz);
    if (err != JVMTI_ERROR_NONE) {
        PLOGE << "Retransform class failed: " << err;
        return nullptr;
    }

    ClassFileData data = classFileDataMap[internalName];
    if (data.classname.empty()) {
        PLOGE << "Failed to capture class [classname null]: " << internalName;
    }
    if (data.bytecode.empty()) {
        PLOGE << "Failed to capture bytecode for class [byetcode null]: " << internalName;
        return nullptr;
    }

    jbyteArray out = env->NewByteArray(data.bytecode.size());
    env->SetByteArrayRegion(out, 0, data.bytecode.size(), (jbyte*)data.bytecode.data());

    return out;
  }

JNIEXPORT jbyteArray JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytesByName
  (JNIEnv *env, jclass loader_class, jstring name) {
    auto& agent = JuiceAgent::Agent::instance();
    if (!check_env(agent) || !name)
        return nullptr;

    // =========================
    // 1. jstring -> internal name
    // =========================
    const char* utf = env->GetStringUTFChars(name, nullptr);
    if (!utf)
        return nullptr;

    std::string internalName(utf);
    env->ReleaseStringUTFChars(name, utf);

    // a.b.C -> a/b/C
    std::replace(internalName.begin(), internalName.end(), '.', '/');

    // =========================
    // 2. Find loaded class via JVMTI
    // =========================
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

            // Lxxx; -> xxx
            if (len > 2 && signature[0] == 'L' && signature[len - 1] == ';') {
                if (internalName == std::string(signature + 1, len - 2)) {
                    targetClass = classes[i];
                    jvmti->Deallocate((unsigned char*)signature);
                    break;
                }
            }

            jvmti->Deallocate((unsigned char*)signature);
        }
    }

    if (classes) {
        jvmti->Deallocate((unsigned char*)classes);
    }

    if (!targetClass) {
        PLOGE << "Class not loaded: " << internalName;
        return nullptr;
    }

    // =========================
    // 3. Reuse existing logic
    // =========================
    return Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_getClassBytes(
        env, loader_class, targetClass
    );
}