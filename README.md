
# JuiceAgent
[English](https://github.com/xiaozhou233/JuiceAgent/blob/main/README.md) | [中文](https://github.com/xiaozhou233/JuiceAgent/blob/main/README_zh.md)


JuiceAgent — a JVMTI-based injection library for loading external JARs and transforming Java bytecode, even when DisableAttachMechanism is enabled.

**NOTICE:** Currently only supports **Windows**, and plans to support **Linux** in the future.

**NOTICE:** **Version 3.1 Build 1 +** is the current **stable release .** Version 3.0 and later have been rewritten and may contain unknown issues.

**WARNING:** This project is still in the experimental stage and is not recommended for production use.
**Use this at your own risk.**

## Summary
JuiceAgent is a JVMTI-native injection framework that injects native code into a target JVM to dynamically load external JARs and perform bytecode redefinition/transformations without requiring a javaagent (works when DisableAttachMechanism=true).
## Features
This project is an implementation of the JuiceAgent-API. For more details, please see [This](https://github.com/xiaozhou233/JuiceAgent-API/blob/master/src/main/java/cn/xiaozhou233/juiceagent/api/JuiceAgent.java)

- addToBootstrapClassLoaderSearch
- addToSystemClassLoaderSearch
- addToClassLoader
- defineClass
- redefineClass(ByName)
- retransformClass(ByName)
- getLoadedClasses
- getClassByName
- getClassBytes(ByName)

## How To Use (**For Version 3.1 Build 2**)

### 1. ⬇️ Download Files 
- Download `libagent.dll` `libinject.dll` `libloader.dll` `injector.exe` from [Release](https://github.com/xiaozhou233/JuiceAgent/releases) and put them in `YourDir`
- Download JARs `JuiceAgent-API-X.X.X+build.X.jar` from [JuiceAgent-API Release] (https://github.com/xiaozhou233/JuiceAgent-API/releases) and put them in `YourDir`

### 2. ✍️ Write Config File
Create a config file named `config.toml` in `YourDir`

```toml
[JuiceAgent]
# Default value: YourDir/JuiceAgent-API.jar
JuiceAgentAPIJarPath = ""
# Default value: YourDir/libagent.dll
JuiceAgentNativePath = ""

[Entry]
# Default value: YourDir/Entry.jar
EntryJarPath = ""
# Default value: com.example.Entry
EntryClass = ""
# Default value: start
EntryMethod = "start"

[Runtime]
# Default value: YourDir/injection
InjectionDir = ""
```

### 3. 💉 Run Injector
#### **Method A** : Use `injector.exe`
Run `injector.exe`
```
114514 loop
Input PID:
```

Input PID of target process and press Enter

#### **Method B** : Use JNI to call `inject` function
Create a file named cn/xiaozhou233/juiceagent/injector.java in your project
```java
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
Then write it down somewhere
```java
// Your class
import cn.xiaozhou233.juiceagent.injector.InjectorNative;

System.load("<path-to-libinject>");
InjectorNative injectorNative = new InjectorNative();

injectorNative.inject(<pid>, "<path-to-libloader>", "<path-to-toml-config-dir>");
```
### 4. ✅ Done!
if injection is successful, target process will load `Entry.jar` (or `EntryJarPath` in config file) and run `EntryClass`.`EntryMethod` method(or `start` method in config file)

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

Dlls will be generated in `build/bin` directory

Jars can be build or download in project [JuiceAgent-API](https://github.com/xiaozhou233/JuiceAgent-API)

## Full Disclaimer/ 完整免责声明
Disclaimer: This project is intended for learning and research in controlled environments only. The author is not responsible for any consequences arising from unauthorized use, modification, cracking, or violation of any applicable agreements or laws. Users are responsible for ensuring their actions are legal and compliant.

免责声明： 本项目仅建议在受控环境中用于学习与研究。任何利用本项目实施破解、篡改、未经授权获取源码或其他违反目标协议或法律的行为所导致的一切后果，作者概不负责。使用者应自行确保其行为合法合规，并对相应后果承担全部责任。

## Acknowledgements

 - [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) - DLL Inject
 - [plog](https://github.com/SergiusTheBest/plog) - portable, simple and extensible C++ logging library
 - [tinytoml](https://github.com/mayah/tinytoml) - A header only C++11 library for parsing TOML.
 - [eventpp](https://github.com/wqking/eventpp) - C++ library for event dispatcher and callback list
