# JuiceAgent
[English](https://github.com/xiaozhou233/JuiceAgent/blob/main/README.md) | [中文](https://github.com/xiaozhou233/JuiceAgent/blob/main/README_zh.md)

JuiceAgent 是一个基于 JVMTI 的注入库，用于加载外部 JAR 并对 Java 字节码进行转换；即使启用了 `DisableAttachMechanism` 也可以使用。

**注意：** 当前仅支持 **Windows**，未来可能会支持 **Linux**。

**注意：** 当前仓库版本为 **Version 3.3 Build 1**。

**警告：** 本项目仍处于实验阶段，不建议在生产环境中使用。使用风险请自行承担。

## 简介
JuiceAgent 是一个基于 JVMTI 的原生注入框架。它通过向目标 JVM 注入原生代码，动态加载外部 JAR，并执行字节码重定义或重转换，而不需要 `javaagent`。在 `DisableAttachMechanism=true` 的情况下依然可以工作。

## 功能特性
本项目是 JuiceAgent-API 的原生实现。API 定义可参考 [JuiceAgent.java](https://github.com/xiaozhou233/JuiceAgent-API/blob/master/src/main/java/cn/xiaozhou233/juiceagent/api/JuiceAgent.java)。

- `addToBootstrapClassLoaderSearch`
- `addToSystemClassLoaderSearch`
- `addToClassLoader`
- `defineClass`
- `redefineClass(ByName)`
- `retransformClass(ByName)`
- `getLoadedClasses`
- `getClassByName`
- `getClassBytes(ByName)`

## 使用方法

### 1. 下载文件
从 [Releases](https://github.com/xiaozhou233/JuiceAgent/releases) 下载以下文件，并放到 `YourDir` 中：

- `libagent.dll`
- `libinject.dll`
- `libloader.dll`
- `injector.exe`

从 [JuiceAgent-API Releases](https://github.com/xiaozhou233/JuiceAgent-API/releases) 下载 `JuiceAgent-API-X.X.X+build.X.jar`，并放到 `YourDir` 中。

### 2. 编写配置文件
在 `YourDir` 下创建 `config.toml`：

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

### 3. 执行注入

#### 方法 A：使用 `injector.exe`
在 `YourDir` 中运行 `injector.exe`。

```text
<jps 输出>
Input PID:
```

输入目标 JVM 进程的 PID 后按回车。

#### 方法 B：通过 JNI 调用 `inject`
在你的项目中创建 `cn/xiaozhou233/juiceagent/injector/InjectorNative.java`：

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
     * @param configDir 包含 config.toml 的目录
     */
    public native boolean inject(int pid, String path, String configDir);
}
```

调用示例：

```java
import cn.xiaozhou233.juiceagent.injector.InjectorNative;

System.load("<path-to-libinject>");
InjectorNative injectorNative = new InjectorNative();

injectorNative.inject(<pid>, "<path-to-libloader>", "<path-to-config-directory>");
```

### 4. 完成
如果注入成功，目标进程会加载 `Entry.jar`（或 `EntryJarPath` 指定的文件），并调用 `EntryClass.EntryMethod`（默认值为 `com.example.Entry.start`）。

## 构建方法

### 环境要求
- Java 8+
- MinGW
- CMake 3.15+
- Ninja

### 使用 CMake Presets 构建
```powershell
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

cmake --preset ninja-release
cmake --build build
```

### 手动构建
```powershell
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

生成的二进制文件位于 `build/bin`。

`JuiceAgent-API.jar` 可以在 [JuiceAgent-API](https://github.com/xiaozhou233/JuiceAgent-API) 项目中构建，或直接从其发布页下载。

## 完整免责声明
本项目仅用于受控环境中的学习与研究。对于任何未经授权的使用、修改、破解，或违反适用协议与法律的行为所产生的后果，作者概不负责。使用者应自行确保其行为合法合规，并承担全部责任。

## 致谢
- [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) - DLL 注入实现
- [plog](https://github.com/SergiusTheBest/plog) - 轻量、可扩展的 C++ 日志库
- [tinytoml](https://github.com/mayah/tinytoml) - Header-only C++11 TOML 解析库
- [eventpp](https://github.com/wqking/eventpp) - 用于事件分发和回调列表的 C++ 库
