# JuiceAgent
[English](https://github.com/xiaozhou233/JuiceAgent/blob/main/README.md) | [中文](https://github.com/xiaozhou233/JuiceAgent/blob/main/README_zh.md)

JuiceAgent —— 一个基于 **JVMTI** 的注入库，用于在 **DisableAttachMechanism 启用的情况下**，向目标 JVM 注入外部 JAR 并对 Java 字节码进行转换。

**注意：** 当前仅支持 **Windows**，未来计划支持 **Linux**。

**警告：** 本项目仍处于实验阶段，不建议用于生产环境。  
**使用风险自负。**

---

## 简介（Summary）

JuiceAgent 是一个基于 **JVMTI 的原生注入框架**，通过向目标 JVM 注入原生代码，实现 **动态加载外部 JAR** 以及 **类重定义 / 类重转换**，并且 **无需使用 javaagent**（即使 `DisableAttachMechanism=true` 也可正常工作）。

---

## 特性（Features）

### JuiceAgent（`src/libagent`）
用于加载 JuiceLoader 的底层注入实现：

- 初始化 JuiceLoader  
- 轻量级库（无需 Java Agent）  
- 读取配置文件并加载 JAR  
- 调用 JuiceLoader Bootstrap API  

---

### libinjector（`src/libinjector`）
注入器模块，支持 **ReflectiveDLLInjection** 与 **CreateRemoteThread** 注入方式：

- 提供 JNI 接口，用于向目标进程注入 DLL  
- 支持通过进程名获取 PID  

---

### JuiceLoader Native（`src/juiceloader`）
[JuiceLoader](https://github.com/xiaozhou233/JuiceLoader) 的原生实现，用于 JAR 注入与类重定义的底层支持。

实现的 Native API 包括：

- `init`
- `injectJar`
- `redefineClass`（`redefineClassByName`）
- `retransformClass`（`retransformClassByName`）
- `getLoadedClasses`
- `getClassBytes`（`getClassBytesByName`）
- `getClassByName`
- `AddToBootstrapClassLoaderSearch`
- `AddToSystemClassLoaderSearch`
- `nativeGetThreadByName`（测试函数，自用）
- `nativeInjectJarToThread`（测试函数，自用）

---

## 使用方法（How To Use，Version 2.3 Build 1）

### 1. 下载文件

- 从 Release 页面下载 `libagent`、`libinjector`、`libjuiceloader`，并放置在 **同一目录**  
- 下载以下 JAR 并放置在同一目录：
  - `bootstrap-api.jar`
  - `JuiceLoader.jar`
  - `Entry.jar`（或你自己的 JAR）

---

### 2. 编写配置文件

在同一目录下创建 `AgentConfig.toml`，示例如下：

```toml
# 留空表示使用默认值
# 默认值：<当前目录>/default-name

# 示例：
# JuiceLoaderJarPath = "D:\Development\JuiceSky\build\libs\JuiceLoader-1.0.jar"

# 默认：<当前目录>/JuiceLoader.jar
JuiceLoaderJarPath = ""

# 默认：<当前目录>/libjuiceloader.dll
JuiceLoaderLibraryPath = ""

# 默认：<当前目录>/injection/
InjectionDir = ""

# 默认：<当前目录>/Entry.jar
EntryJarPath = "D:\Development\JuiceSky\build\libs\JuiceSky-1.0-Hooked.jar"

EntryClass = "cn.xiaozhou233.juicesky.LoaderEntry"
EntryMethod = "start"
```

#### 注入方式说明

| 注入方式 | 说明 |
|---------|------|
| **Reflective 注入** | 使用 `libinjector`，将 **配置文件所在目录** 作为参数传入。<br/><br/>示例：配置文件位于 `<YourDir>/AgentConfig.toml`，则传入 `<YourDir>`。<br/><br/>调用示例：<br/>`C/C++: libinjector.dll -> inject(pid, agentPath, configPath)`<br/>`Java: 见下方步骤` |
| **普通注入** | 使用 `LoadLibraryA + CreateRemoteThread`。<br/>要求 `AgentConfig.toml` 与所有 DLL / JAR 位于同一目录 |

---

### 3. 调用注入函数

#### C / C++

使用 `libinjector`，调用：

```
inject(int pid, char* path, InjectParameters *params)
```

#### Java

使用 `Injecor.jar`：

```
java -jar Injecor.jar
```

或通过 Native 接口：

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

使用示例：

```java
import cn.xiaozhou233.juiceagent.injector.InjectorNative;

System.load("<path-to-libinjector>");
InjectorNative injectorNative = new InjectorNative();

injectorNative.inject(<pid>, "<path-to-libagent>", "<path-to-toml-config-dir>");
```

---

### 4. 完成

如果注入成功，将会执行 `<EntryJarPath>` 中配置的：

```
EntryClass.EntryMethod()
```

---

## 构建方式（How To Build）

### 构建环境

- Java 8+
- MinGW
- CMake
- Ninja

```bash
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

mkdir -p build
cmake --build build --
```

- DLL 输出目录：`build/bin/`  
- JAR 文件：请参考 [JuiceLoader](https://github.com/xiaozhou233/JuiceLoader)

---

## 完整免责声明（Full Disclaimer）

**免责声明（英文）：**  
This project is intended for learning and research in controlled environments only. The author is not responsible for any consequences arising from unauthorized use, modification, cracking, or violation of any applicable agreements or laws. Users are responsible for ensuring their actions are legal and compliant.

**免责声明（中文）：**  
本项目仅建议在受控环境中用于学习与研究。任何利用本项目实施破解、篡改、未经授权获取源码或其他违反目标协议或法律的行为所导致的一切后果，作者概不负责。使用者应自行确保其行为合法合规，并对相应后果承担全部责任。

---

## 致谢（Acknowledgements）

- [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) —— DLL 注入实现  
- [plog](https://github.com/SergiusTheBest/plog) —— 轻量级、可扩展的 C++ 日志库  
- [tinytoml](https://github.com/mayah/tinytoml) —— 仅头文件的 C++11 TOML 解析库  
