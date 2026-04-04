#include <bytecodes_opt.hpp>
#include <jni_impl.hpp>
#include <event_type.hpp>

namespace JuiceAgent::Bytecodes {
    void register_bytecodes() {
        auto& agent = JuiceAgent::Agent::instance();
        
        agent.get_eventbus().subscribe<EventClassFileLoadHook>(capture_bytecodes);
        agent.get_eventbus().subscribe<EventClassFileLoadHook>(patch_bytecodes);
    }

    void capture_bytecodes(const EventClassFileLoadHook& e) {
        /* =========================
             * 1. Capture original bytes
             * ========================= */
            std::lock_guard<std::mutex> lock(classDataMutex);
            if (classToCapture.contains(e.name)) {
                if (!classFileDataMap.contains(e.name)) {
                    ClassFileData data;

                    data.classname = e.name;
                    data.bytecode.assign(e.classbytes, e.classbytes + e.class_data_len);
                    // Experimental: class/classloader/protection_domain references (to be used in future features, e.g. dynamic patching)
                    data.clazz = e.class_being_redefined;
                    data.classloader = e.loader;
                    data.protection_domain = e.protection_domain;


                    classFileDataMap[e.name] = std::move(data);

                    classToCapture.erase(e.name);

                    PLOGI << "Captured class: " << e.name << " (length: " << e.class_data_len << ")";
                }
            }
    }

    void patch_bytecodes(const EventClassFileLoadHook& e) {
        /* =========================
             * 2. Apply patches
            * ========================= */
            if (pendingRetransform.contains(e.name)) {
                auto& bytes = pendingRetransform[e.name];

                unsigned char* new_buf = nullptr;
                e.jvmti_env->Allocate(bytes.size(), &new_buf);

                memcpy(new_buf, bytes.data(), bytes.size());

                *e.new_class_data_len = (jint)bytes.size();
                *e.new_classbytes = new_buf;

                pendingRetransform.erase(e.name);

                PLOGI << "Retransformed: " << e.name << " (new length: " << bytes.size() << ")";
            }
    }
}