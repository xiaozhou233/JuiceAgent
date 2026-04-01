#include <jni_impl.hpp>

// Add jar to bootstrap classloader search path
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_addToBootstrapClassLoaderSearch
  (JNIEnv *env, jclass, jstring jar_path) {
    auto& agent = JuiceAgent::Agent::instance();
    
    if (!check_env(agent)) 
      return JNI_FALSE;

    const char* j_jar_path = env->GetStringUTFChars(jar_path, nullptr);
    if (!j_jar_path) {
        PLOGE << "Path is NULL!";
        return JNI_FALSE;
    }

    jvmtiError result = agent.get_jvmti()->AddToBootstrapClassLoaderSearch(j_jar_path);
    if (result != JVMTI_ERROR_NONE) {
        PLOGE.printf("Cannot AddToBootstrapClassLoaderSearch [result=%d]", result);
        env->ReleaseStringUTFChars(jar_path, j_jar_path);
        return JNI_FALSE;
    }
    PLOGI << "AddToBootstrapClassLoaderSearch: " << j_jar_path;

    env->ReleaseStringUTFChars(jar_path, j_jar_path);
    return JNI_TRUE;
  }

// Add jar to system classloader search path
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_addToSystemClassLoaderSearch
  (JNIEnv *env, jclass, jstring jar_path) {
    auto& agent = JuiceAgent::Agent::instance();
    if (!check_env(agent)) 
      return JNI_FALSE;

    const char* j_jar_path = env->GetStringUTFChars(jar_path, nullptr);
    if (!j_jar_path) {
        PLOGE << "Path is NULL!";
        return JNI_FALSE;
    }

    jvmtiError result = agent.get_jvmti()->AddToSystemClassLoaderSearch(j_jar_path);
    if (result != JVMTI_ERROR_NONE) {
        PLOGE.printf("Cannot AddToSystemClassLoaderSearch [result=%d]", result);
        env->ReleaseStringUTFChars(jar_path, j_jar_path);
        return JNI_FALSE;
    }
    PLOGI << "AddToSystemClassLoaderSearch: " << j_jar_path;

    env->ReleaseStringUTFChars(jar_path, j_jar_path);
    return JNI_TRUE;
  }