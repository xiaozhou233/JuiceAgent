#include <services.hpp>
#include <jni_impl.hpp>

namespace JuiceAgent::services::Bytecode {
    void capture_bytecodes(const EventClassFileLoadHook& e) {
        std::lock_guard<std::mutex> lock(classDataMutex);

        if (!classToCapture.contains(e.name) || classFileDataMap.contains(e.name)) {
            return;
        }

        ClassFileData data;
        data.classname = e.name;
        data.bytecode.assign(e.classbytes, e.classbytes + e.class_data_len);

        if (e.jni_env) {
            if (e.class_being_redefined) data.clazz = static_cast<jclass>(e.jni_env->NewGlobalRef(e.class_being_redefined));
            if (e.loader) data.classloader = e.jni_env->NewGlobalRef(e.loader);
            if (e.protection_domain) data.protection_domain = e.jni_env->NewGlobalRef(e.protection_domain);
        }

        classFileDataMap[e.name] = std::move(data);
        classToCapture.erase(e.name);

        spdlog::info("Captured class: {} (length: {})", e.name, e.class_data_len);
    }

    void patch_bytecodes(const EventClassFileLoadHook& e) {
        std::lock_guard<std::mutex> lock(pendingRetransformMutex);

        auto it = pendingRetransform.find(e.name);
        if (it == pendingRetransform.end()) {
            return;
        }

        auto& bytes = it->second;
        const jint new_len = static_cast<jint>(bytes.size());

        unsigned char* new_buf = nullptr;
        if (e.jvmti_env->Allocate(bytes.size(), &new_buf) != JVMTI_ERROR_NONE) {
            spdlog::error("Failed to allocate buffer for retransform: {}", e.name);
            return;
        }

        memcpy(new_buf, bytes.data(), bytes.size());

        *e.new_class_data_len = new_len;
        *e.new_classbytes = new_buf;

        pendingRetransform.erase(it);

        spdlog::info("Retransformed: {} (new length: {})", e.name, new_len);
    }

    void init() {
        agent().get_eventbus().subscribe<EventClassFileLoadHook>(capture_bytecodes);
        agent().get_eventbus().subscribe<EventClassFileLoadHook>(patch_bytecodes);
    }

    void start() {
        
    }

}