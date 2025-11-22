#include <jni_impl.h>
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_redefineClass
(JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray classBytes, jint length) {
    PLOGI << "Invoked!";
    if (!JuiceLoaderNative.jvmti) {
        PLOGE.printf("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return JNI_FALSE;
    }

    jbyte* buf = env->GetByteArrayElements(classBytes, NULL);
    if (!buf) {
        PLOGE << "Cannot get class bytes";
        return JNI_FALSE;
    }

    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = length;
    defs[0].class_bytes = reinterpret_cast<const unsigned char*>(buf);

    jvmtiError result = JuiceLoaderNative.jvmti->RedefineClasses(1, defs);
    env->ReleaseByteArrayElements(classBytes, buf, JNI_ABORT);

    if (result != JVMTI_ERROR_NONE) {
        PLOGE.printf("Cannot redefine class [result=%d]", result);
        return JNI_FALSE;
    }

    PLOGI.printf("Class redefined successfully!");
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_redefineClassByName
(JNIEnv *env, jclass loader_class, jstring className, jbyteArray classBytes, jint length) {
    PLOGI << "redefineClass_className Invoked!";
    if (!JuiceLoaderNative.jvmti) {
        PLOGE.printf("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return JNI_FALSE;
    }

    const char* cname = env->GetStringUTFChars(className, NULL);
    if (!cname) {
        PLOGE << "Class name is NULL!";
        return JNI_FALSE;
    }

    jclass clazz = env->FindClass(cname);
    if (!clazz) {
        PLOGE.printf("Class not found! [ClassName=%s]", cname);
        env->ReleaseStringUTFChars(className, cname);
        return JNI_FALSE;
    }

    jboolean ret = Java_cn_xiaozhou233_juiceloader_JuiceLoader_redefineClass(env, loader_class, clazz, classBytes, length);
    env->ReleaseStringUTFChars(className, cname);
    return ret;
}