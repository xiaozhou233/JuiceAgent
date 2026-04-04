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