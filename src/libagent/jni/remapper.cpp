#include <jni_impl.hpp>
#include <global.hpp>

JNIEXPORT void JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_addRemapperWhiteList
  (JNIEnv *env, jclass, jstring package_name) {
    std::string pkg_name = env->GetStringUTFChars(package_name, nullptr);
    if (pkg_name.empty()) {
      PLOGW << "package name is empty";
      return;
    }

    remapperWhiteList.insert(pkg_name);

    PLOGI << "add remapper white list: " << pkg_name;
  }

JNIEXPORT void JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_removeRemapperWhiteList
  (JNIEnv *env, jclass, jstring package_name) {
    std::string pkg_name = env->GetStringUTFChars(package_name, nullptr);
    if (pkg_name.empty()) {
      PLOGW << "package name is empty";
      return;
    }

    remapperWhiteList.erase(pkg_name);

    PLOGI << "remove remapper white list: " << pkg_name;
  }

JNIEXPORT void JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_clearRemapperWhiteList
  (JNIEnv *env, jclass) {
    remapperWhiteList.clear();

    PLOGI << "clear remapper white list";
  }
