#include <jni_impl.h>

JNIEXPORT jobject JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_nativeGetThreadByName
  (JNIEnv* env, jclass, jstring jName)
{
    // Convert jstring to std::string
    const char* cName = env->GetStringUTFChars(jName, nullptr);
    std::string targetName = cName;
    env->ReleaseStringUTFChars(jName, cName);

    // Thread class
    jclass threadClass = env->FindClass("java/lang/Thread");

    // Call Thread.getAllStackTraces(), signature: ()Ljava/util/Map;
    jmethodID midGetAll = env->GetStaticMethodID(
        threadClass,
        "getAllStackTraces",
        "()Ljava/util/Map;"
    );

    jobject mapObj = env->CallStaticObjectMethod(threadClass, midGetAll);

    // Map.keySet() -> Set
    jclass mapClass = env->FindClass("java/util/Map");
    jmethodID midKeySet = env->GetMethodID(
        mapClass,
        "keySet",
        "()Ljava/util/Set;"
    );
    jobject setObj = env->CallObjectMethod(mapObj, midKeySet);

    // Set.iterator() -> Iterator
    jclass setClass = env->FindClass("java/util/Set");
    jmethodID midIterator = env->GetMethodID(
        setClass,
        "iterator",
        "()Ljava/util/Iterator;"
    );
    jobject iteratorObj = env->CallObjectMethod(setObj, midIterator);

    // Iterator.hasNext() / next()
    jclass iteratorClass = env->FindClass("java/util/Iterator");
    jmethodID midHasNext = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID midNext = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    // Thread.getName()
    jmethodID midGetName = env->GetMethodID(threadClass, "getName", "()Ljava/lang/String;");

    while (env->CallBooleanMethod(iteratorObj, midHasNext)) {
        jobject threadObj = env->CallObjectMethod(iteratorObj, midNext);
        jstring jThreadName = (jstring)env->CallObjectMethod(threadObj, midGetName);

        const char* cThreadName = env->GetStringUTFChars(jThreadName, nullptr);
        bool match = (targetName == cThreadName);
        env->ReleaseStringUTFChars(jThreadName, cThreadName);

        if (match) {
            return threadObj; // Found target
        }
    }

    return nullptr; // Not found
}

