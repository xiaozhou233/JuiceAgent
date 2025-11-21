#include <jvm/jni.h>
#include <jvm/jvmti.h>
#include <stdio.h>
#include <JuiceAgent/Logger.hpp>
#include <libagent.h>
#include <thread>
#include <tinytoml/toml.h>
#include <JuiceAgent/JuiceAgent.h>

struct _JuiceAgent {
    bool isAttached;
    JavaVM *jvm;
    jvmtiEnv *jvmti;
    JNIEnv *env;
} JuiceAgent = {false, NULL, NULL, NULL};

InjectionInfoType InjectionInfo;

static int ReadConfig(const char* ConfigDir)
{
    // ConfigDir/config.toml
    std::string path = std::string(ConfigDir) + "\\config.toml";
    std::ifstream ifs(path);
    if (!ifs) return -1;

    // Parse toml
    toml::ParseResult pr = toml::parse(ifs);
    if (!pr.valid()) return -2;

    const toml::Value& tbl = pr.value;

    // Helper function: get string from toml
    auto get_str = [&](const char* key, const char* def, char* out) {
        const toml::Value* v = tbl.find(key);
        if (v && v->is<std::string>()) {
            std::string s = v->as<std::string>();
            if (!s.empty()) {
                strncpy(out, s.c_str(), INJECT_PATH_MAX - 1);
                PLOGD << "Found parameter: " << key << " = " << s.c_str();
            } else {
            strncpy(out, def, INJECT_PATH_MAX - 1);
            PLOGD << "Parameter " << key << " is empty, use default: " << def;
            }
        } else {
            strncpy(out, def, INJECT_PATH_MAX - 1);
            PLOGD << "Not found parameter: " << key << ", use default: " << def;
        }
        out[INJECT_PATH_MAX - 1] = '\0';
    };

    // Default values
    std::string defaultJuiceLoader = std::string(ConfigDir) + "\\JuiceLoader.jar";
    std::string defaultJuiceLoaderLibrary = std::string(ConfigDir) + "\\libjuiceloader.dll";
    std::string defaultEntryJar    = std::string(ConfigDir) + "\\Entry.jar";
    std::string defaultInjectionDir = std::string(ConfigDir) + "\\injection";

    get_str("JuiceLoaderJarPath", defaultJuiceLoader.c_str(), InjectionInfo.JuiceLoaderJarPath);
    get_str("JuiceLoaderLibraryPath", defaultJuiceLoaderLibrary.c_str(), InjectionInfo.JuiceLoaderLibraryPath);
    get_str("InjectionDir", defaultInjectionDir.c_str(), InjectionInfo.InjectionDir);
    get_str("EntryJarPath", defaultEntryJar.c_str(), InjectionInfo.EntryJarPath);
    get_str("EntryClass", "cn.xiaozhou233.test.Entry", InjectionInfo.EntryClass);
    get_str("EntryMethod", "start", InjectionInfo.EntryMethod);

    PLOGD << "JuiceLoaderJarPath: " << InjectionInfo.JuiceLoaderJarPath;
    PLOGD << "JuiceLoaderLibraryPath: " << InjectionInfo.JuiceLoaderLibraryPath;
    PLOGD << "InjectionDir: " << InjectionInfo.InjectionDir;
    PLOGD << "EntryJarPath: " << InjectionInfo.EntryJarPath;
    PLOGD << "EntryClass: " << InjectionInfo.EntryClass;
    PLOGD << "EntryMethod: " << InjectionInfo.EntryMethod;

    return 0;
}

static int GetJavaEnv() {
    jint res = JNI_ERR;
    JavaVM *jvm = NULL;
    jvmtiEnv *jvmti = NULL;
    JNIEnv *env = NULL;
    int max_try = 30;

    // Get the JVM
    max_try = 30;
    do {
        res = JNI_GetCreatedJavaVMs(&jvm, 1, NULL);
        if (res != JNI_OK || jvm == NULL) {
            PLOGE.printf("JNI_GetCreatedJavaVMs failed: %d", res);
            PLOGD << "JVM NULL: " << (jvm == NULL);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        max_try--;
        if (jvm != NULL) {
            break;
        } else {
            PLOGI.printf("Waiting for JVM... (%d)", max_try);
        }
    } while (jvm == NULL && max_try > 0);
    if (jvm == NULL) {
        PLOGE << "GetJavaEnv failed";
        return res;
    }
    JuiceAgent.jvm = jvm;
    PLOGI << "Got the JVM";

    // Get the JNIEnv
    max_try = 30;
    do {
        res = jvm->GetEnv((void**)&env, JNI_VERSION_1_8);
        if (res != JNI_OK || env == NULL) {
            PLOGI << "GetEnv failed, trying to attach.";
            PLOGD << "env NULL: " << (env == NULL);
            res = jvm->AttachCurrentThread((void**)&env, NULL);
            if (res != JNI_OK || env == NULL) {
                PLOGE.printf("AttachCurrentThread failed: %d", res);
                PLOGD << "env NULL: " << (env == NULL);
                return res;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        max_try--;
        if (env != NULL) {
            break;
        } else {
            PLOGI.printf("Waiting for JNI... (%d)", max_try);
        }
    } while (env == NULL && max_try > 0);
    if (env == NULL) {
        PLOGE << "Get JNI failed";
        return res;
    }
    JuiceAgent.env = env;
    PLOGI << "Got the JNIEnv";
    
    // Get the JVMTI
    res = jvm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        PLOGE.printf("GetEnv failed: %d", res);
        PLOGD << "jvmti NULL: " << (jvmti == NULL);
        return res;
    }
    JuiceAgent.jvmti = jvmti;
    PLOGI << "Got the JVMTI";

    return 0;
}

static int InvokeJuiceLoaderInit(const char* ConfigDir) {
    // Variables
    int result = JNI_ERR;
    JNIEnv *env = JuiceAgent.env;
    jvmtiEnv *jvmti = JuiceAgent.jvmti;

    const char *bootstrap_class = "cn/xiaozhou233/juiceloader/JuiceLoaderBootstrap";
    const char *method_name = "init";
    const char *method_signature = "([Ljava/lang/String;)V";

    PLOGI << "Invoke JuiceLoader Init";
    PLOGI << "================ JuiceLoader Init =================";
    // Find class
    jclass cls = env->FindClass(bootstrap_class);
    if (cls == NULL) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        PLOGE << "FindClass failed";
        return 1;
    }

    // Find method
    jmethodID mid = env->GetStaticMethodID(cls, method_name, method_signature);
    if (mid == NULL) {
        PLOGE << "GetStaticMethodID failed";
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return 1;
    }

    // arguments array
    const char* args[] = {
        InjectionInfo.EntryJarPath,
        InjectionInfo.EntryClass,
        InjectionInfo.EntryMethod,
        InjectionInfo.InjectionDir,
        InjectionInfo.JuiceLoaderLibraryPath
    };
    int argCount = sizeof(args)/sizeof(args[0]);
    
    jclass stringCls = env->FindClass("java/lang/String");
    jobjectArray jArgs = env->NewObjectArray(argCount, stringCls, nullptr);
    for (int i = 0; i < argCount; i++) {
        env->SetObjectArrayElement(jArgs, i, env->NewStringUTF(args[i]));
    }

    // Invoke method
    env->CallStaticVoidMethod(cls, mid, jArgs);
    if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
    }

    PLOGI << "================ JuiceLoader Init =================";

    PLOGI << "Invoke JuiceLoader Init done";
    return 0;
}

extern "C" __declspec(dllexport)
bool InitJuiceAgent(const char* runtime_path) {
    PLOGI << "--------> InitJuiceAgent Invoke <--------";
    PLOGD << "runtime_path: " << runtime_path;

    // Read Config
    PLOGI << "ReadConfig ...";
    if (ReadConfig(runtime_path) != 0) {
        PLOGE << "ReadConfig failed";
        return false;
    }
    PLOGI << "[OK] ReadConfig";
    
    // Get JavaEnv
    PLOGI << "Get JavaEnv ...";
    if (GetJavaEnv() != 0) {
        PLOGE << "GetJavaEnv failed";
        return false;
    }
    PLOGI << "[OK] Get JavaEnv";

    // Inject JuiceLoader jar
    PLOGI << "Inject JuiceLoader jar ...";
    if (JuiceAgent.jvmti->AddToSystemClassLoaderSearch(InjectionInfo.JuiceLoaderJarPath) != JNI_OK) {
        PLOGE << "AddToSystemClassLoaderSearch failed";
        return false;
    }
    PLOGI << "[OK] Inject JuiceLoader jar";

    // Invoke JuiceLoader Init
    PLOGI << "Invoke JuiceLoader Init ...";
    if (InvokeJuiceLoaderInit(runtime_path) != 0) {
        PLOGE << "Invoke JuiceLoader Init failed";
        return false;
    }
    PLOGI << "[OK] Invoke JuiceLoader Init";

    // Load JuiceLoaderLibrary

    PLOGI << "--------> InitJuiceAgent Succeed <--------";
    
    return true;
}