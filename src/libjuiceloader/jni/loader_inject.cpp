#include <jni_impl.h>

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_injectJar
(JNIEnv *env, jclass loader_class, jstring jarPath) {
    PLOGI << "Invoked!";
    const char *pathStr = env->GetStringUTFChars(jarPath, NULL);
    if (!pathStr) {
        PLOGE << "Path is NULL!";
        return JNI_FALSE;
    }

    if (!JuiceLoaderNative.jvmti) {
        PLOGE.printf("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        env->ReleaseStringUTFChars(jarPath, pathStr);
        return JNI_FALSE;
    }

    jvmtiError result = JuiceLoaderNative.jvmti->AddToSystemClassLoaderSearch(pathStr);
    if (result != JVMTI_ERROR_NONE) {
        PLOGE.printf("Cannot inject jar [result=%d]", result);
        env->ReleaseStringUTFChars(jarPath, pathStr);
        return JNI_FALSE;
    }

    env->ReleaseStringUTFChars(jarPath, pathStr);
    PLOGI << "Jar injected: " << pathStr;
    return JNI_TRUE;
}


JNIEXPORT jobject JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_nativeInjectJarToThread
 (JNIEnv* env, jclass, jobject threadObj, jstring jJarPath)
{
    // Convert jstring -> std::string
    const char* cJarPath = env->GetStringUTFChars(jJarPath, nullptr);

    // new File(jarPath)
    jclass fileClass = env->FindClass("java/io/File");
    jmethodID fileCtor = env->GetMethodID(fileClass, "<init>", "(Ljava/lang/String;)V");
    jstring jPathCopy = env->NewStringUTF(cJarPath);
    jobject fileObj = env->NewObject(fileClass, fileCtor, jPathCopy);

    // File.toURI()
    jmethodID midToURI = env->GetMethodID(fileClass, "toURI", "()Ljava/net/URI;");
    jobject uriObj = env->CallObjectMethod(fileObj, midToURI);

    // URI.toURL()
    jclass uriClass = env->FindClass("java/net/URI");
    jmethodID midToURL = env->GetMethodID(uriClass, "toURL", "()Ljava/net/URL;");
    jobject urlObj = env->CallObjectMethod(uriObj, midToURL);

    // Thread.getContextClassLoader()
    jclass threadClass = env->FindClass("java/lang/Thread");
    jmethodID midGetCL = env->GetMethodID(
        threadClass, "getContextClassLoader", "()Ljava/lang/ClassLoader;"
    );
    jobject originalCL = env->CallObjectMethod(threadObj, midGetCL);

    // URLClassLoader(URL[] urls, ClassLoader parent)
    jclass urlClassLoaderClass = env->FindClass("java/net/URLClassLoader");
    jmethodID urlCLCtor = env->GetMethodID(
        urlClassLoaderClass,
        "<init>",
        "([Ljava/net/URL;Ljava/lang/ClassLoader;)V"
    );

    // Create URL[] array with 1 element
    jobjectArray urlArray = env->NewObjectArray(1, env->FindClass("java/net/URL"), nullptr);
    env->SetObjectArrayElement(urlArray, 0, urlObj);

    // Create new URLClassLoader
    jobject newClassLoader = env->NewObject(
        urlClassLoaderClass,
        urlCLCtor,
        urlArray,
        originalCL
    );

    // thread.setContextClassLoader(newCL)
    jmethodID midSetCL = env->GetMethodID(
        threadClass,
        "setContextClassLoader",
        "(Ljava/lang/ClassLoader;)V"
    );
    env->CallVoidMethod(threadObj, midSetCL, newClassLoader);

    env->ReleaseStringUTFChars(jJarPath, cJarPath);

    return newClassLoader;
}