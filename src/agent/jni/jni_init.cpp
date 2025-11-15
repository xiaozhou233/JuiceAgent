#include <cn_xiaozhou233_juiceloader_JuiceLoader.h>
#include <JuiceAgent/Logger.hpp>
#include <libagent.h>

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_init
  (JNIEnv *, jclass) {
    InitLogger();
    PLOGI << "init";

    PLOGI << "Invoke Agent EntryPoint...";

    EntryPoint();
    return true;
  }