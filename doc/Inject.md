# Inject Method

## Method A: Use `injector.exe`
Run `injector.exe` from `YourDir`.

```text
<jps output>
Input PID:
```

Enter the PID of the target JVM process and press Enter.

## Method B: Use JNI to call `inject`
Download the JuiceAgent-JavaInjector-x.x-xxxx.jar from [here](https://github.com/xiaozhou233/JuiceAgent-JavaInjector/releases).

Add JuiceAgent-JavaInjector-x.x-xxxx.jar to your classpath.

Example:

```java
import cn.xiaozhou233.juiceagent.injector.Injector;

System.load("<path-to-libinject>");

Injector.inject(<pid>, "<path-to-libloader>", "<path-to-config-directory>");
```
