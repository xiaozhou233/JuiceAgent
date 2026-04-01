#include <jni_impl.hpp>

// Redefine class
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClass
  (JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray class_bytes, jint class_bytes_len) {
    auto& agent = JuiceAgent::Agent::instance();
    if (!check_env(agent))
        return JNI_FALSE;

    jbyte* buf = env->GetByteArrayElements(class_bytes, nullptr);
    if (!buf) {
        PLOGE << "Cannot get class bytes";
        return JNI_FALSE;
    }

    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = class_bytes_len;
    defs[0].class_bytes = reinterpret_cast<const unsigned char*>(buf);

    jvmtiError result = agent.get_jvmti()->RedefineClasses(1, defs);
    env->ReleaseByteArrayElements(class_bytes, buf, JNI_ABORT);

    if (result != JVMTI_ERROR_NONE) {
        PLOGE << "Redefine class failed: " << result;
        return JNI_FALSE;
    }

    PLOGD << "Class redefined successfully!";

    return JNI_TRUE;
  }

// Redefine class using class_name
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClassByName
  (JNIEnv *env, jclass loader_class, jstring class_name, jbyteArray class_bytes, jint class_bytes_len) {
    // jstring -> const char*
    const char* cname = env->GetStringUTFChars(class_name, nullptr);
    if (!cname) {
        PLOGE << "Class name is NULL!";
        return JNI_FALSE;
    }

    // Find class by name
    jclass clazz = env->FindClass(cname);
    env->ReleaseStringUTFChars(class_name, cname);
    if (!clazz) {
        PLOGE << "Cannot find class: " << cname;
        return JNI_FALSE;
    }

    return Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_redefineClass(env, loader_class, clazz, class_bytes, class_bytes_len);
}