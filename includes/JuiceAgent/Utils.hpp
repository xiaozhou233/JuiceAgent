#pragma once

#include <jni.h>
#include <jvmti.h>
#include <JuiceAgent/Logger.hpp>

// Check for JNI exceptions and clear them
bool check_and_clear_exception(JNIEnv* env, const char* context) {
    if (env->ExceptionCheck()) {
        PLOGE << "JNI Exception occurred at: " << context;
        env->ExceptionDescribe();
        env->ExceptionClear();
        return true;
    }
    return false;
}