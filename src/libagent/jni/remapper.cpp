#include <jni_impl.hpp>
#include <global.hpp>

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_addBlacklist
  (JNIEnv *env, jclass, jstring j_pkg) {

    // jstring -> std::string
    const char* pkg = env->GetStringUTFChars(j_pkg, NULL);
    std::string str_pkg(pkg);
    env->ReleaseStringUTFChars(j_pkg, pkg);

    // add to blacklist
    remapperBlackList.insert(str_pkg);

    PLOGD << "Added to remapper blacklist: " << str_pkg;
 return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_addWhitelist
  (JNIEnv *env, jclass, jstring j_pkg) {
    // jstring -> std::string
    const char* pkg = env->GetStringUTFChars(j_pkg, NULL);
    std::string str_pkg(pkg);
    env->ReleaseStringUTFChars(j_pkg, pkg);

    // add to whitelist
    remapperWhiteList.insert(str_pkg);

    PLOGD << "Added to remapper whitelist: " << str_pkg;
 return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_removeBlacklist
  (JNIEnv *env, jclass, jstring j_pkg) {
    // jstring -> std::string
    const char* pkg = env->GetStringUTFChars(j_pkg, NULL);
    std::string str_pkg(pkg);
    env->ReleaseStringUTFChars(j_pkg, pkg);

    // remove from blacklist
    remapperBlackList.erase(str_pkg);

    PLOGD << "Removed from remapper blacklist: " << str_pkg;
 return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_removeWhitelist
  (JNIEnv *env, jclass, jstring j_pkg) {
    // jstring -> std::string
    const char* pkg = env->GetStringUTFChars(j_pkg, NULL);
    std::string str_pkg(pkg);
    env->ReleaseStringUTFChars(j_pkg, pkg);

    // remove from whitelist
    remapperWhiteList.erase(str_pkg);

    PLOGD << "Removed from remapper whitelist: " << str_pkg;
 return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_clearBlacklist
  (JNIEnv *env, jclass) {
    // clear blacklist
    remapperBlackList.clear();

    PLOGD << "Cleared remapper blacklist";
    return JNI_TRUE;
  }

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceremapper_JuiceRemapper_clearWhitelist
  (JNIEnv *env, jclass) {
    // clear whitelist
    remapperWhiteList.clear();

    PLOGD << "Cleared remapper whitelist";
    return JNI_TRUE;
  }
