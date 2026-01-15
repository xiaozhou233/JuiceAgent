#include <jni_impl.h>

JNIEXPORT jclass JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_defineClass
  (JNIEnv *env, jclass, jobject cl, jbyteArray bytes) {
    PLOGD << "Invoked!";
    jclass clClass = env->FindClass("java/lang/ClassLoader");
    jmethodID defineClass = env->GetMethodID(clClass, "defineClass", "([BII)Ljava/lang/Class;");
    jobject classDefined = env->CallObjectMethod(cl, defineClass, bytes, 0,
                                                    env->GetArrayLength(bytes));
    return (jclass)classDefined;
}