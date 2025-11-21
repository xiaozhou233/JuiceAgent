#include <jvm/jni.h>
#include <jvm/jvmti.h>
#include <stdio.h>

JNIEXPORT jint JNICALL 
Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    printf("Agent loaded\n");
    return JNI_OK;
}