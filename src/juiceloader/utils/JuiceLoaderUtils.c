#include "JuiceLoaderUtils.h"

/* Helper: get method full name: ClassName.methodName(signature) */
char* get_method_fullname(jvmtiEnv *j, JNIEnv *env, jthread thread, jmethodID method) {
    char *mname = NULL, *msig = NULL, *mgeneric = NULL;
    jclass declaring = NULL;
    char *cls_sig = NULL;
    char *result = NULL;

    jvmtiError err;

    err = (*j)->GetMethodName(j, method, &mname, &msig, &mgeneric);
    if (err != JVMTI_ERROR_NONE) goto cleanup;

    err = (*j)->GetMethodDeclaringClass(j, method, &declaring);
    if (err != JVMTI_ERROR_NONE) goto cleanup;

    err = (*j)->GetClassSignature(j, declaring, &cls_sig, NULL);
    if (err != JVMTI_ERROR_NONE) goto cleanup;

    /* cls_sig is like "Lcom/example/MyClass;" -> make it human-friendly */
    size_t len = strlen(cls_sig) + strlen(mname) + strlen(msig) + 4;
    result = (char*)malloc(len);
    if (result) {
        /* remove leading L and trailing ; from class signature if present */
        char *cls_nice = cls_sig;
        if (cls_sig[0] == 'L') cls_nice = cls_sig + 1;
        size_t cls_len = strlen(cls_nice);
        if (cls_nice[cls_len - 1] == ';') cls_nice[cls_len - 1] = '\0';
        snprintf(result, len, "%s.%s%s", cls_nice, mname, msig);
    }

cleanup:
    if (mname) (*j)->Deallocate(j, (unsigned char*)mname);
    if (msig) (*j)->Deallocate(j, (unsigned char*)msig);
    if (mgeneric) (*j)->Deallocate(j, (unsigned char*)mgeneric);
    if (cls_sig) (*j)->Deallocate(j, (unsigned char*)cls_sig);
    return result;
}
