#include <cn_xiaozhou233_juiceloader_JuiceLoader.h>
#include <JuiceAgent/Logger.hpp>

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_init
  (JNIEnv *, jclass) {
    InitLogger();
    PLOGI << "init";
    return true;
  }