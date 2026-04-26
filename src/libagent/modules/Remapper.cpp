#include <modules/ModuleRegistry.hpp>
#include <JuiceAgent/Logger.hpp>
#include <libagent.hpp>
#include <event/event_type.hpp>
#include <global.hpp>
#include <string_view>
#include <string>
#include <JuiceAgent/Utils.hpp>

namespace JuiceAgent::Core::Modules {

struct RemapperConfig {
    bool enabled = false;
    std::string JuiceRemapperJarPath;
};

static RemapperConfig config;
static JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();

class RemapperModule : public IModule {
    struct _Remapper {
        jclass handler_class = nullptr;
        jmethodID onClassFileLoadMethod = nullptr;
        jmethodID onMethodEntryMethod = nullptr;
        jmethodID onMethodExitMethod = nullptr;
    } Remapper;

public:
    std::string name() const override {
        return "Remapper";
    }

    bool init() override {
        if (_initialized) {
            return true;
        }

        auto& cfg = agent.get_config();

        config.enabled = cfg.get<bool>(
            "JuiceAgent.Modules.Remapper.Enabled",
            false
        );

        config.JuiceRemapperJarPath = cfg.get<std::string>(
            "JuiceAgent.Modules.Remapper.JuiceRemapperJarPath",
            "./JuiceRemapper.jar",
            true
        );

        if (!config.enabled) {
            _initialized = true;
            return true;
        }

        if (agent.get_jvmti()->AddToSystemClassLoaderSearch(
            config.JuiceRemapperJarPath.c_str()
        ) != JNI_OK) {
            PLOGE << "Failed to load JuiceRemapper.jar";
            return false;
        }

        JNIEnv* env = agent.get_env();

        jclass local = env->FindClass(
            "cn/xiaozhou233/juiceremapper/api/Handler"
        );

        if (local == nullptr) {
            check_and_clear_exception(env, "FindClass Handler");
            PLOGE << "Failed to find Handler";
            return false;
        }

        Remapper.handler_class =
            static_cast<jclass>(env->NewGlobalRef(local));

        env->DeleteLocalRef(local);

        if (Remapper.handler_class == nullptr) {
            return false;
        }

        Remapper.onClassFileLoadMethod = env->GetStaticMethodID(
            Remapper.handler_class,
            "onClassFileLoadHook",
            "(Ljava/lang/Class;Ljava/lang/ClassLoader;Ljava/lang/String;Ljava/lang/Object;I[B)[B"
        );

        if (Remapper.onClassFileLoadMethod == nullptr) {
            check_and_clear_exception(env, "onClassFileLoadHook");
            return false;
        }

        Remapper.onMethodEntryMethod = env->GetStaticMethodID(
            Remapper.handler_class,
            "onMethodEntry",
            "(Ljava/lang/Thread;Ljava/lang/reflect/Method;)V"
        );

        if (Remapper.onMethodEntryMethod == nullptr) {
            check_and_clear_exception(env, "onMethodEntry");
            return false;
        }

        Remapper.onMethodExitMethod = env->GetStaticMethodID(
            Remapper.handler_class,
            "onMethodExit",
            "(Ljava/lang/Thread;Ljava/lang/reflect/Method;ZLjava/lang/Object;)V"
        );

        if (Remapper.onMethodExitMethod == nullptr) {
            check_and_clear_exception(env, "onMethodExit");
            return false;
        }

        _initialized = true;
        return true;
    }

    bool start() override {
        if (!_initialized) {
            return false;
        }

        if (_running) {
            return true;
        }

        if (!config.enabled) {
            _running = true;
            return true;
        }

        auto& bus = agent.get_eventbus();

        _classFileToken = bus.subscribe<EventClassFileLoadHook>(
            [this](const auto& e) {
                on_class_file_load_hook(e);
            }
        );

        _methodEntryToken = bus.subscribe<EventMethodEntry>(
            [this](const auto& e) {
                on_method_entry_hook(e);
            }
        );

        _methodExitToken = bus.subscribe<EventMethodExit>(
            [this](const auto& e) {
                on_method_exit_hook(e);
            }
        );

        _running = true;
        return true;
    }

    void stop() override {
        if (!_running) {
            return;
        }

        auto& bus = agent.get_eventbus();

        bus.unsubscribe<EventClassFileLoadHook>(_classFileToken);
        bus.unsubscribe<EventMethodEntry>(_methodEntryToken);
        bus.unsubscribe<EventMethodExit>(_methodExitToken);

        _classFileToken = 0;
        _methodEntryToken = 0;
        _methodExitToken = 0;

        _running = false;
    }

private:
    void on_class_file_load_hook(const EventClassFileLoadHook& event) {
        std::string_view name = event.name ? event.name : "";

        if (name.starts_with("cn/xiaozhou233/juiceremapper/")) {
            return;
        }

        JNIEnv* env = event.jni_env;

        std::string className(name);
        jstring jName = env->NewStringUTF(className.c_str());
        if (jName == nullptr) {
            return;
        }

        jbyteArray input = env->NewByteArray(event.class_data_len);
        if (input == nullptr) {
            env->DeleteLocalRef(jName);
            return;
        }

        env->SetByteArrayRegion(
            input,
            0,
            event.class_data_len,
            reinterpret_cast<const jbyte*>(event.classbytes)
        );

        jbyteArray output = static_cast<jbyteArray>(
            env->CallStaticObjectMethod(
                Remapper.handler_class,
                Remapper.onClassFileLoadMethod,
                event.class_being_redefined,
                event.loader,
                jName,
                event.protection_domain,
                event.class_data_len,
                input
            )
        );

        check_and_clear_exception(env, "on_class_file_load_hook");

        if (output != nullptr) {
            jsize len = env->GetArrayLength(output);
            unsigned char* buffer = nullptr;

            if (event.jvmti_env->Allocate(len, &buffer) == JVMTI_ERROR_NONE) {
                env->GetByteArrayRegion(
                    output,
                    0,
                    len,
                    reinterpret_cast<jbyte*>(buffer)
                );

                *event.new_class_data_len = len;
                *event.new_classbytes = buffer;
            }

            env->DeleteLocalRef(output);
        }

        env->DeleteLocalRef(input);
        env->DeleteLocalRef(jName);
    }
    void on_method_entry_hook(const EventMethodEntry& event) {
        // event.jni_env->CallStaticVoidMethod(
        //     Remapper.handler_class,
        //     Remapper.onMethodEntryMethod,
        //     event.thread,
        //     event.method
        // );

        check_and_clear_exception(event.jni_env, "on_method_entry_hook");
    }

    void on_method_exit_hook(const EventMethodExit& event) {
        // event.jni_env->CallStaticVoidMethod(
        //     Remapper.handler_class,
        //     Remapper.onMethodExitMethod,
        //     event.thread,
        //     event.method,
        //     event.was_popped_by_exception,
        //     event.return_value.l
        // );

        check_and_clear_exception(event.jni_env, "on_method_exit_hook");
    }

private:
    bool _initialized = false;
    bool _running = false;

    EventBus::Token _classFileToken = 0;
    EventBus::Token _methodEntryToken = 0;
    EventBus::Token _methodExitToken = 0;
};

}

JUICEAGENT_REGISTER_MODULE(
    JuiceAgent::Core::Modules::RemapperModule,
    Remapper
)