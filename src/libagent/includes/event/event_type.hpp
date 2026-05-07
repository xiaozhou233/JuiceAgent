#pragma once

#include <jni_impl.hpp>

// ClassFileLoadHook
struct EventClassFileLoadHook {
    jvmtiEnv* jvmti_env;
    JNIEnv* jni_env;
    jclass class_being_redefined;
    jobject loader;
    const char* name;
    jobject protection_domain;
    jint class_data_len;
    const unsigned char* classbytes;
    jint* new_class_data_len;
    unsigned char** new_classbytes;
};

// MethodEntry
struct EventMethodEntry {
    jvmtiEnv* jvmti_env;
    JNIEnv* jni_env;
    jthread thread;
    jmethodID method;
};

// MethodExit
struct EventMethodExit {
    jvmtiEnv* jvmti_env;
    JNIEnv* jni_env;
    jthread thread;
    jmethodID method;
    jboolean was_popped_by_exception;
    jvalue return_value;
};

// JuiceAgent PreLoad/PostLoad
struct EventPreLoad {
    JavaVM* jvm;
    JNIEnv* env;
    jvmtiEnv* jvmti;
};

struct EventLoaded {
    JavaVM* jvm;
    JNIEnv* env;
    jvmtiEnv* jvmti;
};