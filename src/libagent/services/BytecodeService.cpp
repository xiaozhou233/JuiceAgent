#include <services.hpp>
#include <jni_impl.hpp>

namespace JuiceAgent::services::Bytecode {
        // Release the JNI global references held by a ClassFileData entry.
        // Call before erasing/replacing an entry in classFileDataMap to avoid leaks.
        static void release_class_file_data(JNIEnv* env, ClassFileData& data) {
            if (!env) return;

            if (data.clazz) {
                env->DeleteGlobalRef(data.clazz);
                data.clazz = nullptr;
            }
            if (data.classloader) {
                env->DeleteGlobalRef(data.classloader);
                data.classloader = nullptr;
            }
            if (data.protection_domain) {
                env->DeleteGlobalRef(data.protection_domain);
                data.protection_domain = nullptr;
            }
        }

        void capture_bytecodes(const EventClassFileLoadHook& e) {
        /* =========================
             * 1. Capture original bytes
             * ========================= */
            if (!e.jni_env) return;

            std::lock_guard<std::mutex> lock(classDataMutex);
            if (classToCapture.contains(e.name)) {
                // Refresh the cached entry if this class was captured before
                // (e.g. re-capture after a redefinition).
                auto it = classFileDataMap.find(e.name);
                if (it != classFileDataMap.end()) {
                    release_class_file_data(e.jni_env, it->second);
                    classFileDataMap.erase(it);
                }

                ClassFileData data;

                data.classname = e.name;
                data.bytecode.assign(e.classbytes, e.classbytes + e.class_data_len);
                // Store global references so they stay valid after the JVMTI
                // callback returns (raw local refs would dangle immediately).
                data.clazz = e.class_being_redefined
                    ? static_cast<jclass>(e.jni_env->NewGlobalRef(e.class_being_redefined)) : nullptr;
                data.classloader = e.loader
                    ? e.jni_env->NewGlobalRef(e.loader) : nullptr;
                data.protection_domain = e.protection_domain
                    ? e.jni_env->NewGlobalRef(e.protection_domain) : nullptr;

                classFileDataMap[e.name] = std::move(data);

                classToCapture.erase(e.name);

                PLOGI << "Captured class: " << e.name << " (length: " << e.class_data_len << ")";
            }
    }

    void patch_bytecodes(const EventClassFileLoadHook& e) {
        /* =========================
             * 2. Apply patches
            * ========================= */
            std::vector<unsigned char> bytes;
            {
                std::lock_guard<std::mutex> lock(pendingRetransformMutex);

                auto it = pendingRetransform.find(e.name);
                if (it == pendingRetransform.end()) return;

                bytes = std::move(it->second);
                pendingRetransform.erase(it);
            }

            unsigned char* new_buf = nullptr;
            jvmtiError err = e.jvmti_env->Allocate(bytes.size(), &new_buf);
            if (err != JVMTI_ERROR_NONE || new_buf == nullptr) {
                PLOGE << "Failed to allocate buffer for retransform: " << e.name
                      << " (err=" << err << ")";
                return;
            }

            memcpy(new_buf, bytes.data(), bytes.size());

            *e.new_class_data_len = (jint)bytes.size();
            *e.new_classbytes = new_buf;

            PLOGI << "Retransformed: " << e.name << " (new length: " << bytes.size() << ")";
    }

    void init() {
        agent.get_eventbus().subscribe<EventClassFileLoadHook>(capture_bytecodes);
        agent.get_eventbus().subscribe<EventClassFileLoadHook>(patch_bytecodes);
    }

    void start() {
        
    }

}