#include <JuiceAgent/Logger.hpp>
#include <libloader.hpp>
#include <config.hpp>
#include <jvm.hpp>

namespace libloader
{
    // 累了，这几把的代码扔给AI算了 气死了
    bool invoke_juiceagent_init(JNIEnv* env, const InjectionInfo& info) {
        std::string bootstrap_class = "cn/xiaozhou233/juiceagent/api/JuiceAgentBootstrap";
        std::string method_name = "start";
        std::string method_desc = "([Ljava/lang/String;)V";

        // Find Class
        jclass bootstrap_class_obj = env->FindClass(bootstrap_class.c_str());
        if (!bootstrap_class_obj) {
            if (env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            PLOGE << "Failed to find JuiceAgentBootstrap class: " << bootstrap_class;
            return false;
        }

        // Find Method
        jmethodID method_id = env->GetStaticMethodID(bootstrap_class_obj, method_name.c_str(), method_desc.c_str());
        if (!method_id) {
            if (env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            PLOGE << "Failed to find method: " << method_name;
            return false;
        }

        // Prepare arguments (order matters)
        const char* args[] = {
            info.EntryJarPath.c_str(),
            info.EntryClass.c_str(),
            info.EntryMethod.c_str(),
            info.InjectionDir.c_str(),
            info.JuiceAgentNativePath.c_str()
        };
        int argCount = sizeof(args) / sizeof(args[0]);

        jclass stringCls = env->FindClass("java/lang/String");
        jobjectArray jArgs = env->NewObjectArray(argCount, stringCls, nullptr);
        for (int i = 0; i < argCount; i++) {
            env->SetObjectArrayElement(jArgs, i, env->NewStringUTF(args[i]));
        }

        // Invoke the method
        env->CallStaticVoidMethod(bootstrap_class_obj, method_id, jArgs);
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        // Cleanup
        env->DeleteLocalRef(jArgs);
        env->DeleteLocalRef(stringCls);

        PLOGI << "JuiceAgentBootstrap.start invoked successfully";
        return true;
    }


    void entrypoint(const char* runtime_dir)
    {
        if (runtime_dir == nullptr) {
            PLOGE << "runtime_dir is null, fallback may fail";
        }

        // Load Config (allow fallback)
        config::Config cfg(runtime_dir);
        if (!cfg.is_valid()) {
            PLOGW << "Invalid config, using default values";
        }

        InjectionInfo info = cfg.get_injection_info();
        config::print_injection_info(info);

        // Validate injection path early
        if (info.JuiceAgentAPIJarPath.empty()) {
            PLOGE << "JuiceAgentAPIJarPath is empty, abort injection";
            return;
        }

        // Attach to JVM
        jvm::Jvm jvm;
        if (!jvm.attach()) {
            PLOGE << "Attach to JVM failed";
            return;
        }

        auto* jvmti = jvm.get_jvmti();
        if (jvmti == nullptr) {
            PLOGE << "JVMTI is null";
            return;
        }

        // Inject JuiceAgentAPI.jar
        const char* jar_path = info.JuiceAgentAPIJarPath.c_str();
        jint status = jvmti->AddToSystemClassLoaderSearch(jar_path);

        if (status != JNI_OK) {
            PLOGE << "AddToSystemClassLoaderSearch failed: " << status;
            return;
        }

        PLOGI << "Jar injected successfully: " << jar_path;

        invoke_juiceagent_init(jvm.get_env(), info);
    }
} // namespace libloader