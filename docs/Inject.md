# Inject Method

## Method A: Use `injector.exe`
Run `injector.exe` from `YourDir`.

```text
<jps output>
Input PID:
```

Enter the PID of the target JVM process and press Enter.

## Method B: Use JNI to call `inject`
Create `cn/xiaozhou233/juiceagent/injector/InjectorNative.java` in your project:

```java
package cn.xiaozhou233.juiceagent.injector;

public class InjectorNative {
    /*
     * @param pid target process id
     * @param path injection DLL path
     */
    public native boolean inject(int pid, String path);

    /*
     * @param pid target process id
     * @param path injection DLL path
     * @param configDir directory containing config.toml
     */
    public native boolean inject(int pid, String path, String configDir);
}
```

Example:

```java
import cn.xiaozhou233.juiceagent.injector.InjectorNative;

System.load("<path-to-libinject>");
InjectorNative injectorNative = new InjectorNative();

injectorNative.inject(<pid>, "<path-to-libloader>", "<path-to-config-directory>");
```
