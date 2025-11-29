
# JuiceAgent
[English]() | [中文]()

JuiceAgent — a JVMTI-based injection library for loading external JARs and transforming Java bytecode, even when DisableAttachMechanism is enabled.

**NOTICE:** Currently only supports **Windows**, and plans to support **Linux** in the future.

**WARNING:** This project is still in the experimental stage and is not recommended for production use.
**Use this at your own risk.**

## Summary
JuiceAgent is a JVMTI-native injection framework that injects native code into a target JVM to dynamically load external JARs and perform bytecode redefinition/transformations without requiring a javaagent (works when DisableAttachMechanism=true).
## Features
### JuiceAgent (src/libagent)
 - Initialize JuiceLoader
 - Lightweight Library (No Javaagent)
 - Read Config File and Load JARs
 - Invoke JuiceLoader Bootstrap API

### libinjector (src/libinjector)
 - Provides JNI functions for injecting DLLs into target processes
 - Get PID by process name

### JuiceLoader Native (src/juiceloader)
 Native library for [JuiceLoader](https://github.com/xiaozhou233/JuiceLoader) , Inject Jars/ Redefine class Low-level implementation.
- Implements JuiceLoader's native API
    - init
    - injectJar
    - redefineClass (redefineClassByName)
    - retransformClass (retransformClassByName)
    - getLoadedClasses
    - getClassBytes (getClassBytesByName)
    - getClassByName
    - nativeGetThreadByName* (test function, myself-use)
    - nativeInjectJarToThread (test function, myself-use)
## How To Use (For Version 2.2 Build 1)

### 1. Download Files
- Download `libagent`, `libinjector`, and `libjuiceloader` from the Release to the same directory.  
- Download JARs `bootstrap-api.jar`, `JuiceLoader.jar`, and `Entry.jar` (or your own JAR) to the same directory.

### 2. Write Config File
Create a config file named `AgentConfig.toml` in the same directory. Example:

```toml
# Leave empty to use default values
# Default values: <Current Directory>/default-name

# Example: JuiceLoaderJarPath = "D:\\Development\\JuiceSky\\build\\libs\\JuiceLoader-1.0.jar"
# Default: <Current Directory>/JuiceLoader.jar
JuiceLoaderJarPath = ""
# Default: <Current Directory>/libjuiceloader.dll
JuiceLoaderLibraryPath = ""
# Default: <Current Directory>/injection/
InjectionDir = ""
# Default: <Current Directory>/Entry.jar
EntryJarPath = "D:\\Development\\JuiceSky\\build\\libs\\JuiceSky-1.0-Hooked.jar"
EntryClass = "cn.xiaozhou233.juicesky.LoaderEntry"
EntryMethod = "start"
```
| Method| Usage|
| - | -|
| **Reflective Inject** | Uses `libinjector`. Pass the config directory as a parameter. </br></br> Example: config file at `<YourDir>/AgentConfig.toml`, </br></br>pass `<YourDir>` as parameter.</br></br> Example:</br>`C/C++:  libinjector.dll -> inject(pid, agentpath, configpath)`</br>`Java: See 3. Invoke Inject Function`|
| **Normal Inject**     | Uses `LoadLibraryA + CreateRemoteThread`. Keep `AgentConfig.toml` and all DLLs/JARs in the same directory.</br>

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

## Full Disclaimer/ 完整免责声明
Disclaimer: This project is intended for learning and research in controlled environments only. The author is not responsible for any consequences arising from unauthorized use, modification, cracking, or violation of any applicable agreements or laws. Users are responsible for ensuring their actions are legal and compliant.

免责声明： 本项目仅建议在受控环境中用于学习与研究。任何利用本项目实施破解、篡改、未经授权获取源码或其他违反目标协议或法律的行为所导致的一切后果，作者概不负责。使用者应自行确保其行为合法合规，并对相应后果承担全部责任。

## Acknowledgements

 - [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) - DLL Inject
 - [plog](https://github.com/SergiusTheBest/plog) - portable, simple and extensible C++ logging library
 - [tinytoml](https://github.com/mayah/tinytoml) - A header only C++11 library for parsing TOML.

