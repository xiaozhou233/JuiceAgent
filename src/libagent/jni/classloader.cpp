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

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_addToClassLoader
  (JNIEnv *env, jclass, jstring jar_path, jobject target_classloader) {

    auto& agent = JuiceAgent::Agent::instance();

    if (!check_env(agent))
        return JNI_FALSE;

    if (!jar_path || !target_classloader)
        return JNI_FALSE;

    // --- Try URLClassLoader path ---
    jclass urlClassLoader = env->FindClass("java/net/URLClassLoader");
    if (env->ExceptionCheck() || !urlClassLoader) {
        env->ExceptionClear();
        goto FALLBACK;
    }

    if (env->IsInstanceOf(target_classloader, urlClassLoader)) {

        jclass fileClass = env->FindClass("java/io/File");
        if (env->ExceptionCheck() || !fileClass) {
            env->ExceptionClear();
            goto FALLBACK;
        }

        jmethodID fileInit = env->GetMethodID(fileClass, "<init>", "(Ljava/lang/String;)V");
        if (env->ExceptionCheck() || !fileInit) {
            env->ExceptionClear();
            goto FALLBACK;
        }

        jobject file = env->NewObject(fileClass, fileInit, jar_path);
        if (env->ExceptionCheck() || !file) {
            env->ExceptionClear();
            goto FALLBACK;
        }

        jmethodID toURI = env->GetMethodID(fileClass, "toURI", "()Ljava/net/URI;");
        if (env->ExceptionCheck() || !toURI) {
            env->ExceptionClear();
            env->DeleteLocalRef(file);
            goto FALLBACK;
        }

        jobject uri = env->CallObjectMethod(file, toURI);
        if (env->ExceptionCheck() || !uri) {
            env->ExceptionClear();
            env->DeleteLocalRef(file);
            goto FALLBACK;
        }

        jclass uriClass = env->FindClass("java/net/URI");
        if (env->ExceptionCheck() || !uriClass) {
            env->ExceptionClear();
            env->DeleteLocalRef(file);
            env->DeleteLocalRef(uri);
            goto FALLBACK;
        }

        jmethodID toURL = env->GetMethodID(uriClass, "toURL", "()Ljava/net/URL;");
        if (env->ExceptionCheck() || !toURL) {
            env->ExceptionClear();
            env->DeleteLocalRef(file);
            env->DeleteLocalRef(uri);
            goto FALLBACK;
        }

        jobject url = env->CallObjectMethod(uri, toURL);
        if (env->ExceptionCheck() || !url) {
            env->ExceptionClear();
            env->DeleteLocalRef(file);
            env->DeleteLocalRef(uri);
            goto FALLBACK;
        }

        jmethodID addURL = env->GetMethodID(urlClassLoader, "addURL", "(Ljava/net/URL;)V");
        if (env->ExceptionCheck() || !addURL) {
            env->ExceptionClear();
            env->DeleteLocalRef(file);
            env->DeleteLocalRef(uri);
            env->DeleteLocalRef(url);
            goto FALLBACK;
        }

        env->CallVoidMethod(target_classloader, addURL, url);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            env->DeleteLocalRef(file);
            env->DeleteLocalRef(uri);
            env->DeleteLocalRef(url);
            goto FALLBACK;
        }

        // cleanup
        env->DeleteLocalRef(file);
        env->DeleteLocalRef(uri);
        env->DeleteLocalRef(url);

        return JNI_TRUE;
    }

FALLBACK:

    // --- JVMTI fallback ---
    const char* j_jar_path = env->GetStringUTFChars(jar_path, nullptr);
    if (!j_jar_path)
        return JNI_FALSE;

    jvmtiError err = agent.get_jvmti()->AddToSystemClassLoaderSearch(j_jar_path);

    env->ReleaseStringUTFChars(jar_path, j_jar_path);

    if (err != JVMTI_ERROR_NONE) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

// TODO: Safety and Compatibility: Define Class
// Define class using target classloader
JNIEXPORT jclass JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_defineClass
  (JNIEnv *env, jclass, jobject target_classloader, jbyteArray bytes) {
    auto& agent = JuiceAgent::Agent::instance();

    if (!check_env(agent))
        return nullptr;
    
    jclass clClass = env->FindClass("java/lang/ClassLoader");
    jmethodID defineClass = env->GetMethodID(clClass, "defineClass", "([BII)Ljava/lang/Class;");
    jobject classDefined = env->CallObjectMethod(target_classloader, defineClass, bytes, 0,
                                                    env->GetArrayLength(bytes));
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }
    return (jclass)classDefined;
  }