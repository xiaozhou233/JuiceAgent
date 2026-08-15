#include <jni_impl.hpp>
#include <event/event_type.hpp>

void JNICALL MethodEntry(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method) {
                auto& agent = JuiceAgent::Agent::instance();
                agent.get_eventbus().post(EventMethodEntry{
                    .jvmti_env = jvmti_env,
                    .jni_env = jni_env,
                    .thread = thread,
                    .method = method
                });
            }
