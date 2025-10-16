package cn.xiaozhou233.juiceagent.injector;

public class InjectorNative {
    /*
     * @param pid target process id
     */
    public native boolean inject(int pid);

    /*
     * @param pid target process id
     * @param path libinjector path
     */
    public native boolean inject(int pid, String path);
}