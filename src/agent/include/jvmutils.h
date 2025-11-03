#ifndef GLOBALUTILS_H
#define GLOBALUTILS_H

#include "windows.h"
#include "stdio.h"
#include "jni.h"
#include "jvmti.h"

jint GetCreatedJVM(JavaVM**);
jint GetJNIEnv(JavaVM*, JNIEnv**);
jint GetJVMTI(JavaVM*, jvmtiEnv**);

#endif