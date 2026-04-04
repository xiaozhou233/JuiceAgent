#include <jni_impl.hpp>
#include <event_type.hpp>

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
                PLOGW << "name is null";
                return;
            }

            auto& agent = JuiceAgent::Agent::instance();
            agent.get_eventbus().post(EventClassFileLoadHook{
                .jvmti_env = jvmti_env,
                .jni_env = jni_env,
                .class_being_redefined = class_being_redefined,
                .loader = loader,
                .name = name,
                .protection_domain = protection_domain,
                .class_data_len = class_data_len,
                .classbytes = classbytes,
                .new_class_data_len = new_class_data_len,
                .new_classbytes = new_classbytes
            });
        }