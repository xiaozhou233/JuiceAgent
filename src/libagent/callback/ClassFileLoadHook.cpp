#include <jni_impl.hpp>
#include <iostream>

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
            // Delete this log when the hook is stable, or it will cause performance issues
            std::cout << "ClassFileLoadHook: " << name << std::endl;
            if (!name) {
                PLOGW << "name is null";
                return;
            }

            /* =========================
             * 1. Capture original bytes
             * ========================= */
            std::lock_guard<std::mutex> lock(classDataMutex);
            if (classToCapture.contains(name)) {
                if (!classFileDataMap.contains(name)) {
                    ClassFileData data;

                    data.classname = name;
                    data.bytecode.assign(classbytes, classbytes + class_data_len);
                    // Experimental: class/classloader/protection_domain references (to be used in future features, e.g. dynamic patching)
                    data.clazz = class_being_redefined;
                    data.classloader = loader;
                    data.protection_domain = protection_domain;


                    classFileDataMap[name] = std::move(data);

                    classToCapture.erase(name);

                    PLOGI << "Captured class: " << name << " (length: " << class_data_len << ")";
                }
            }
        }