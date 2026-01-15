#ifndef JNI_IMPL_H
#define JNI_IMPL_H

#include <jni.h>
#include <jvmti.h>
#include <juiceloader.h>
#include <JuiceAgent/Logger.hpp>
#include <cn_xiaozhou233_juiceloader_JuiceLoader.h>

// Variables
extern std::string g_target_internal_name_str;
extern const char* g_target_internal_name;
extern std::vector<unsigned char> g_bytecodes;
extern RetransformClassCacheType RetransformClassCache;

void JNICALL ClassFileLoadHook(
        jvmtiEnv* jvmti_env,
        JNIEnv* jni_env,
        jclass class_being_redefined,
        jobject loader,
        const char* name,
        jobject protection_domain,
        jint class_data_len,
        const unsigned char* classbytes,
        jint* new_class_data_len,
        unsigned char** new_classbytes);

#endif // JNI_IMPL_H