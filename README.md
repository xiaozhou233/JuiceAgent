
# JuiceAgent
[English]() | [中文]()

JuiceAgent — a JVMTI-based injection library for loading external JARs and transforming Java bytecode, even when DisableAttachMechanism is enabled.

**NOTICE:** Currently only supports **Windows**, and plans to support **Linux** in the future.
## Summary
JuiceAgent is a JVMTI-native injection framework that injects native code into a target JVM to dynamically load external JARs and perform bytecode redefinition/transformations without requiring a javaagent (works when DisableAttachMechanism=true).
## Features
### Injector (src/injector)
 Reflective injector, can be invoke in java (java/InjectorNative.java)
- Inject Dlls. (with no params)
- Inject JuiceAgent library. (with params)

### JuiceAgent (src/agent)
 Minimize loading, inject to target jvm.
- Get JVM / JNI / JVMTI
- Inject bootstrap-api.jar
- Inject JuiceLoader.jar
- Invoke Java Function JuiceLoader init

### JuiceLoader Native (src/juiceloader)
 Native library for [JuiceLoader](https://github.com/xiaozhou233/JuiceLoader) , Inject Jars/ Redefine class Low-level implementation.
- Get JVM / JNI / JVMTI
- Inject Custom Jars
- Redefine class
- Get Loaded Classes
- JNI Function (Can be invoke in java)
## How To Use

### 1. Download Files
- Download `libagent`, `libinjector`, and `libjuiceloader` from the Release to the same directory.  
- Download JARs `bootstrap-api.jar`, `JuiceLoader.jar`, and `Entry.jar` (or your own JAR) to the same directory.

### 2. Write Config File
Create a config file named `AgentConfig.toml` in the same directory. Example:

```toml
[Injection]
BootstrapAPIPath = "<YourDir>/bootstrap-api.jar"
JuiceLoaderJarPath = "<YourDir>/JuiceLoader.jar"
# Leave empty to use the DLL directory (Normal Inject) or the directory passed as parameter (Reflective Inject)
JuiceLoaderLibPath = ""
EntryJarPath = "<YourDir>/Entry.jar"
EntryClass = "entry.class"
EntryMethod = "entryMethod"
```
| Method| Usage|
| - | -|
| **Reflective Inject** | Uses `libinjector`. Pass the config directory as a parameter. </br> Example: config file at `<YourDir>/AgentConfig.toml`, pass `<YourDir>` as parameter. If a key is empty (e.g., `BootstrapAPIPath = ""`), it defaults to `<DirA>/default-name` (e.g., `<DirA>/bootstrap-api.jar`). |
| **Normal Inject**     | Uses `LoadLibraryA + CreateRemoteThread`. Keep `AgentConfig.toml` and all DLLs/JARs in the same directory.

### 3. Invoke Inject Function
 C/C++ Use libinjector and invoke inject(int pid, char* path, InjectParameters *params)

 Java Use `Injecor.jar`: `java -jar Injecor.jar`  or
 ``` 
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
```
```
// Your class
import cn.xiaozhou233.juiceagent.injector.InjectorNative;

System.load("<path-to-libinjector>");
InjectorNative injectorNative = new InjectorNative();

injectorNative.inject(<pid>, "<path-to-libagent>", "<path-to-toml-config-dir>");
```

### 4. Done!
If the injection is successful, EntryClass .EntryMethod() of <EntryJarPath> will be executed.
## Hot To Build
### Environment
- Java 8+
- MinGW
- CMake
- Ninja
```
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

mkdir -p build
cmake --build build --
```
dlls: build/bin/

jars: see [JuiceLoader](https://github.com/xiaozhou233/JuiceLoader)
## Acknowledgements

 - [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) - DLL Inject
 - [log.c](https://github.com/rxi/log.c) - A simple logger system
 - [tomlc17](https://github.com/cktan/tomlc17) - TOML parser in C17 

