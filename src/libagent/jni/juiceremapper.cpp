#include <jni_common.hpp>
#include <global.hpp>

static const char* remap_class = "cn/xiaozhou233/juiceremapper/remapper/Remap";
static const char* remap_method = "remap";
static const char* remap_sig = "(Ljava/lang/String;[B)[B";

namespace {

// Resolve and cache the static class/method declarations.
// Returns true on success. On failure, caches are left null so the
// next call retries the lookup.
bool cacheRemapDeclarations(JNIEnv* env) {
    auto& remapper = JuiceAgent::Services::Remapper::getInstance();

    if (remapper.getRemapClass() && remapper.getRemapMethodId()) {
        return true;
    }

    jclass localClass = env->FindClass(remap_class);
    if (!localClass) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        spdlog::error("[JuiceRemapper JNI] FindClass failed: {}", remap_class);
        return false;
    }

    jclass globalClass = static_cast<jclass>(env->NewGlobalRef(localClass));
    env->DeleteLocalRef(localClass);
    if (!globalClass) {
        spdlog::error("[JuiceRemapper JNI] NewGlobalRef failed: {}", remap_class);
        return false;
    }

    jmethodID methodId = env->GetStaticMethodID(globalClass, remap_method, remap_sig);
    if (!methodId) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteGlobalRef(globalClass);
        spdlog::error("[JuiceRemapper JNI] GetStaticMethodID failed: {}.{}",
                      remap_class, remap_method);
        return false;
    }

    remapper.setRemapClass(globalClass);
    remapper.setRemapMethodId(methodId);
    spdlog::debug("[JuiceRemapper JNI] Cached static declarations: {}.{}",
                  remap_class, remap_method);
    return true;
}

} // namespace

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_initNative
  (JNIEnv *env, jclass) {
    // TODO: Init

    auto& remapper = JuiceAgent::Services::Remapper::getInstance();
    if (remapper.isInit()) {
        spdlog::warn("JuiceRemapper::initNative: Remapper is already initialized");
        return JNI_FALSE;
    }

    if (!cacheRemapDeclarations(env)) {
        spdlog::error("JuiceRemapper::initNative: failed to cache static declarations");
        return JNI_FALSE;
    }

    remapper.setInit(true);

    spdlog::info("[JuiceRemapper JNI] JuiceRemapper is initialized.");

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_addInclude
  (JNIEnv *env, jclass, jstring name) {
    if (!name) return JNI_FALSE;

    const char* utf = env->GetStringUTFChars(name, nullptr);
    if (!utf) return JNI_FALSE;
    std::string s(utf);
    env->ReleaseStringUTFChars(name, utf);

    JuiceAgent::Services::Remapper::getInstance().addInclude(std::move(s));
    spdlog::debug("[JuiceRemapper JNI] addInclude");
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_addExclude
  (JNIEnv *env, jclass, jstring name) {
    if (!name) return JNI_FALSE;

    const char* utf = env->GetStringUTFChars(name, nullptr);
    if (!utf) return JNI_FALSE;
    std::string s(utf);
    env->ReleaseStringUTFChars(name, utf);

    JuiceAgent::Services::Remapper::getInstance().addExclude(std::move(s));
    spdlog::debug("[JuiceRemapper JNI] addExclude");
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_removeInclude
  (JNIEnv *env, jclass, jstring name) {
    if (!name) return JNI_FALSE;

    const char* utf = env->GetStringUTFChars(name, nullptr);
    if (!utf) return JNI_FALSE;
    std::string s(utf);
    env->ReleaseStringUTFChars(name, utf);

    JuiceAgent::Services::Remapper::getInstance().removeInclude(s);
    spdlog::debug("[JuiceRemapper JNI] removeInclude");
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_removeExclude
  (JNIEnv *env, jclass, jstring name){
    if (!name) return JNI_FALSE;

    const char* utf = env->GetStringUTFChars(name, nullptr);
    if (!utf) return JNI_FALSE;
    std::string s(utf);
    env->ReleaseStringUTFChars(name, utf);

    JuiceAgent::Services::Remapper::getInstance().removeExclude(s);
    spdlog::debug("[JuiceRemapper JNI] removeExclude");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_clearIncludes
  (JNIEnv *, jclass) {
    JuiceAgent::Services::Remapper::getInstance().clearIncludes();
    spdlog::debug("[JuiceRemapper JNI] clearIncludes");
}

JNIEXPORT void JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_clearExcludes
  (JNIEnv *, jclass) {
    JuiceAgent::Services::Remapper::getInstance().clearExcludes();
    spdlog::debug("[JuiceRemapper JNI] clearExcludes");
}