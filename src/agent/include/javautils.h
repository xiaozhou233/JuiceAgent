#ifndef JAVAUTILS_H
#define JAVAUTILS_H

#include <jni.h>
#include <jvmti.h>

char* get_class_name(JNIEnv* env, jclass clazz);

#endif //JAVAUTILS_H