#include <modules/ModuleRegistry.hpp>
#include <modules/ModuleBase.hpp>
#include <JuiceAgent/Logger.hpp>
#include <libagent.hpp>
#include <event/eventbus.hpp>
#include <event/event_type.hpp>

#include <string>
#include <string_view>
#include <cstring>
#include <unordered_set>

#include <global.hpp>

namespace JuiceAgent::Core::Modules {

struct RemapperConfig {
    bool enabled = false;
};

struct RemapperBridge {
    static jclass bridge_class;
    static jmethodID remap_method;
};

jclass RemapperBridge::bridge_class = nullptr;
jmethodID RemapperBridge::remap_method = nullptr;

static JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();
static RemapperConfig config;

// ===== match result =====
struct MatchResult {
    size_t length = 0;   // length of matched prefix
    bool matched = false;
};

// ===== find longest prefix match =====
static MatchResult match_longest(
    const std::unordered_set<std::string>& list,
    std::string_view name
) {
    MatchResult result;

    for (const auto& prefix : list) {
        if (name.starts_with(prefix)) {
            if (!result.matched || prefix.length() > result.length) {
                result.matched = true;
                result.length = prefix.length();
            }
        }
    }

    return result;
}

// ===== final decision =====
static bool should_remap(std::string_view name) {
    auto white = match_longest(remapperWhiteList, name);
    auto black = match_longest(remapperBlackList, name);

    // no match at all -> deny
    if (!white.matched && !black.matched) {
        return false;
    }

    // only white matched
    if (white.matched && !black.matched) {
        return true;
    }

    // only black matched
    if (!white.matched && black.matched) {
        return false;
    }

    // both matched -> choose longer prefix
    if (white.length >= black.length) {
        return true;   // whitelist wins
    } else {
        return false;  // blacklist wins (more specific)
    }
}

// recursion guard
static thread_local bool in_remap = false;

class RemapperModule : public ModuleBase {
public:
    std::string name() const override {
        return "Remapper";
    }

protected:
    bool on_init() override {
        auto& cfg = agent.get_config();

        config.enabled = cfg.get<bool>(
            "JuiceAgent.Modules.Remapper.Enabled",
            false
        );

        if (!config.enabled) {
            return true;
        }

        JNIEnv* env = agent.get_env();

        jclass local_class = env->FindClass(
            "cn/xiaozhou233/juiceremapper/bridge/JuiceAgentBridge"
        );
        if (!local_class) {
            PLOGE << "Failed to find JuiceAgentBridge";
            return false;
        }

        RemapperBridge::bridge_class =
            reinterpret_cast<jclass>(env->NewGlobalRef(local_class));
        env->DeleteLocalRef(local_class);

        RemapperBridge::remap_method = env->GetStaticMethodID(
            RemapperBridge::bridge_class,
            "remap",
            "(Ljava/lang/Class;Ljava/lang/Object;Ljava/lang/String;Ljava/lang/Object;I[B)[B"
        );
        if (!RemapperBridge::remap_method) {
            PLOGE << "Failed to find remap method";
            return false;
        }

        return true;
    }

    bool on_start() override {
        if (!config.enabled) {
            return true;
        }

        auto& bus = agent.get_eventbus();

        _classFileToken = bus.subscribe_mutable<EventClassFileLoadHook>(
            [this](auto& e) {
                on_class_file_load_hook(e);
            }
        );

        return true;
    }

    void on_stop() override {
        if (!config.enabled) return;

        auto& bus = agent.get_eventbus();

        if (_classFileToken != 0) {
            bus.unsubscribe<EventClassFileLoadHook>(_classFileToken);
            _classFileToken = 0;
        }

        JNIEnv* env = agent.get_env();
        if (RemapperBridge::bridge_class) {
            env->DeleteGlobalRef(RemapperBridge::bridge_class);
            RemapperBridge::bridge_class = nullptr;
        }
    }

private:
    void on_class_file_load_hook(EventClassFileLoadHook& event) {
        if (!event.classbytes || event.class_data_len <= 0) {
            return;
        }

        std::string_view name = event.name ? event.name : "";
        if (name.empty()) return;

        if (in_remap) return;

        // ===== apply whitelist/blacklist decision =====
        if (!should_remap(name)) {
            return;
        }

        JNIEnv* env = event.jni_env;
        jvmtiEnv* jvmti = event.jvmti_env;

        if (!env || !jvmti) return;

        in_remap = true;

        jbyteArray inputArray = env->NewByteArray(event.class_data_len);
        if (!inputArray) {
            in_remap = false;
            return;
        }

        env->SetByteArrayRegion(
            inputArray,
            0,
            event.class_data_len,
            reinterpret_cast<const jbyte*>(event.classbytes)
        );

        jstring jname = env->NewStringUTF(name.data());

        jobject result = env->CallStaticObjectMethod(
            RemapperBridge::bridge_class,
            RemapperBridge::remap_method,
            nullptr,
            event.loader,
            jname,
            event.protection_domain,
            0,
            inputArray
        );

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            cleanup(env, inputArray, jname, nullptr);
            in_remap = false;
            return;
        }

        if (!result) {
            cleanup(env, inputArray, jname, nullptr);
            in_remap = false;
            return;
        }

        jbyteArray outputArray = reinterpret_cast<jbyteArray>(result);

        jsize new_len = env->GetArrayLength(outputArray);

        jbyte* new_bytes = env->GetByteArrayElements(outputArray, nullptr);
        if (!new_bytes) {
            cleanup(env, inputArray, jname, outputArray);
            in_remap = false;
            return;
        }

        unsigned char* new_class_data = nullptr;

        if (jvmti->Allocate(new_len, &new_class_data) != JVMTI_ERROR_NONE) {
            env->ReleaseByteArrayElements(outputArray, new_bytes, JNI_ABORT);
            cleanup(env, inputArray, jname, outputArray);
            in_remap = false;
            return;
        }

        std::memcpy(new_class_data, new_bytes, new_len);

        *event.new_classbytes = new_class_data;
        *event.new_class_data_len = new_len;

        env->ReleaseByteArrayElements(outputArray, new_bytes, JNI_ABORT);
        cleanup(env, inputArray, jname, outputArray);

        in_remap = false;
    }

    void cleanup(JNIEnv* env, jbyteArray in, jstring name, jbyteArray out) {
        if (in) env->DeleteLocalRef(in);
        if (name) env->DeleteLocalRef(name);
        if (out) env->DeleteLocalRef(out);
    }

private:
    EventBus::Token _classFileToken = 0;
};

JUICEAGENT_REGISTER_MODULE(
    JuiceAgent::Core::Modules::RemapperModule,
    RemapperModule
)

} // namespace