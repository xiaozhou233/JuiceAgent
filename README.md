# JuiceAgent
[English](https://github.com/xiaozhou233/JuiceAgent/blob/main/README.md) | [中文](https://github.com/xiaozhou233/JuiceAgent/blob/main/README_zh.md)

JuiceAgent is a JVMTI-based injection library for loading external JARs and transforming Java bytecode, even when `DisableAttachMechanism` is enabled.

**Notice:** Currently, only **Windows** is supported. **Linux** support may be added in the future.

**Notice:** The current repository version is **Version 3.3 Build 1**.

**Warning:** This project is still experimental and is not recommended for production use. Use it at your own risk.

## Summary
JuiceAgent is a JVMTI-native injection framework that injects native code into a target JVM to dynamically load external JARs and perform bytecode redefinition or retransformation without requiring a `javaagent`. It can still work when `DisableAttachMechanism=true`.

## Features
This project is the native implementation of JuiceAgent-API. For the API definition, see [JuiceAgent.java](https://github.com/xiaozhou233/JuiceAgent-API/blob/master/src/main/java/cn/xiaozhou233/juiceagent/api/JuiceAgent.java).

- `addToBootstrapClassLoaderSearch`
- `addToSystemClassLoaderSearch`
- `addToClassLoader`
- `defineClass`
- `redefineClass(ByName)`
- `retransformClass(ByName)`
- `getLoadedClasses`
- `getClassByName`
- `getClassBytes(ByName)`

## How To Use

### 1. Download Files
Download the following files from [Releases](https://github.com/xiaozhou233/JuiceAgent/releases) and place them in `YourDir`:

- `libagent.dll`
- `libinject.dll`
- `libloader.dll`
- `injector.exe`

Download `JuiceAgent-API-X.X.X+build.X.jar` from [JuiceAgent-API Releases](https://github.com/xiaozhou233/JuiceAgent-API/releases) and place it in `YourDir`.

### 2. Write the Config File
Create a file named `config.toml` in `YourDir`.

```toml
[JuiceAgent]
Version = 1

[JuiceAgent.Loader]
JuiceAgentAPIJarPath = ""
JuiceAgentNativeLibraryPath = ""

[JuiceAgent.Modules]

[JuiceAgent.Modules.JarLoader]
Enabled = true
InjectionDir = ""
JarPath = ""
EntryClass = ""
EntryMethod = ""
```

### 3. Run the Injector

#### Method A: Use `injector.exe`
Run `injector.exe` from `YourDir`.

```text
<jps output>
Input PID:
```

Enter the PID of the target JVM process and press Enter.

#### Method B: Use JNI to call `inject`
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

### 4. Done
If the injection succeeds, the target process will load `Entry.jar` (or the file specified by `EntryJarPath`) and invoke `EntryClass.EntryMethod` (default: `com.example.Entry.start`).

## How To Build

### Requirements
- Java 8+
- MinGW
- CMake 3.15+
- Ninja

### Build with CMake Presets
```powershell
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

cmake --preset ninja-release
cmake --build build
```

### Manual Build
```powershell
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The generated binaries will be placed in `build/bin`.

`JuiceAgent-API.jar` can be built from or downloaded from [JuiceAgent-API](https://github.com/xiaozhou233/JuiceAgent-API).

## Full Disclaimer
This project is intended for learning and research in controlled environments only. The author is not responsible for any consequences arising from unauthorized use, modification, cracking, or violation of any applicable agreements or laws. Users are responsible for ensuring that their actions are legal and compliant.

## Acknowledgements
- [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) - DLL injection implementation
- [plog](https://github.com/SergiusTheBest/plog) - Portable, simple, and extensible C++ logging library
- [tinytoml](https://github.com/mayah/tinytoml) - Header-only C++11 TOML parser
- [eventpp](https://github.com/wqking/eventpp) - C++ library for event dispatchers and callback lists
