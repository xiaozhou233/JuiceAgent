#include <jni_common.hpp>

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
        unsigned char** new_classbytes) {
            if (!name) {
                spdlog::warn("[ClassFileLoadHook] name is null");
                return;
            }

            // TODO: Impl & Delete this
            spdlog::trace("[ClassFileLoadHook] name: {}", name);
        }