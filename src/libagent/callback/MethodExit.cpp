#include <jni_impl.hpp>
#include <event/event_type.hpp>

void JNICALL MethodExit(jvmtiEnv *jvmti_env,
            JNIEnv* jni_env,
            jthread thread,
            jmethodID method,
            jboolean was_popped_by_exception,
            jvalue return_value) {
                auto& agent = JuiceAgent::Agent::instance();
                agent.get_eventbus().post(EventMethodExit{
                    .jvmti_env = jvmti_env,
                    .jni_env = jni_env,
                    .thread = thread,
                    .method = method,
                    .was_popped_by_exception = was_popped_by_exception,
                    .return_value = return_value
                });
            }