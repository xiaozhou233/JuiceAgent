#include <jni_common.hpp>
#include <eventbus.hpp>

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
            
            EventClassFileLoadHook event{
                jvmti_env,
                jni_env,
                class_being_redefined,
                loader,
                name,
                protection_domain,
                class_data_len,
                classbytes,
                new_class_data_len,
                new_classbytes
            };

            EventBus::getInstance().post(EventId::ClassFileLoadHook, &event);
        }