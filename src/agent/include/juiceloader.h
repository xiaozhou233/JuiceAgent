#ifndef JUICELOADER_H
#define JUICELOADER_H

#include <jni.h>
#include <jvmti.h>

typedef struct _JuiceLoaderNative {
    jvmtiEnv *jvmti;
} JuiceLoaderNativeType;
extern JuiceLoaderNativeType JuiceLoaderNative;

jint init_juiceloader(JNIEnv *env, jvmtiEnv *jvmti);

// JNI Functions
// public native static boolean init();
JNIEXPORT jboolean JNICALL loader_init(JNIEnv *env, jclass loader_class);
// public native static boolean injectJar(String jarPath);
JNIEXPORT jboolean JNICALL loader_injectJar(JNIEnv *env, jclass loader_class, jstring jarPath);
// public native static boolean redefineClass(Class<?> clazz, byte[] classBytes, int length);
JNIEXPORT jboolean JNICALL loader_redefineClass_clazz(JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray classBytes, jint length);
// public native static boolean redefineClass(String className, byte[] classBytes, int length);
JNIEXPORT jboolean JNICALL loader_redefineClass_className(JNIEnv *env, jclass loader_class, jstring className, jbyteArray classBytes, jint length);
// public native static Class<?>[] getLoadedClasses();
JNIEXPORT jobjectArray JNICALL loader_getLoadedClasses(JNIEnv *env, jclass loader_class);


#endif