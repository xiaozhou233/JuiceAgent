package cn.xiaozhou233.juiceagent.injector;

public class InjectorNative {
    /*
     * @param pid target process id
     */
    public native boolean inject(int pid, String path);

    /*
     * @param pid target process id
     * @param path injection dll path
     * @param configPath config file path
     */
    public native boolean inject(int pid, String path, String configPath);
}