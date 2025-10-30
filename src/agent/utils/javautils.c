#include "javautils.h"
#include <jni.h>
#include <jvmti.h>
#include <string.h>

char* get_class_name(JNIEnv* env, jclass clazz) {
    jclass clsClass = (*env)->FindClass(env, "java/lang/Class");
    jmethodID mid_getName = (*env)->GetMethodID(env, clsClass, "getName", "()Ljava/lang/String;");
    jstring nameStr = (jstring)(*env)->CallObjectMethod(env, clazz, mid_getName);

    const char* temp = (*env)->GetStringUTFChars(env, nameStr, NULL);
    char* result = strdup(temp);
    (*env)->ReleaseStringUTFChars(env, nameStr, temp);
    (*env)->DeleteLocalRef(env, nameStr);
    return result;
}