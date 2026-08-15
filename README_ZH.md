# JuiceAgent

[English](README.md) | [简体中文](README_ZH.md)

> **注意**：本文档由 AI 自动翻译，可能存在不准确或不通顺之处，请以 [英文版 README](README.md) 为准。如有疑问欢迎提 issue 或 PR 修正。

JuiceAgent 是一个基于 JVMTI 的高级注入框架，用于运行时 JAR 加载、字节码变换和运行时 JVM 插桩。它可以在 `DisableAttachMechanism=true` 的情况下注入到正在运行的 JVM 进程中，无需 `-javaagent` 启动参数，也无需使用 attach 机制。

**警告**：本项目仍处于实验阶段，不建议用于生产环境。使用风险自负。

> **提示**：部分杀毒软件可能将 JuiceAgent 误报为恶意软件。详情见[杀毒软件误报说明](docs/Antivirus-False-Positive.md)。

**注意**：本项目针对标准 JVM 设计，自定义 JVM 可能无法运行。

![License](https://img.shields.io/github/license/xiaozhou233/JuiceAgent)
![Release](https://img.shields.io/github/v/release/xiaozhou233/JuiceAgent)
![GitHub last commit](https://img.shields.io/github/last-commit/xiaozhou233/JuiceAgent)
![GitHub repo size](https://img.shields.io/github/repo-size/xiaozhou233/JuiceAgent)
![Downloads](https://img.shields.io/github/downloads/xiaozhou233/JuiceAgent/total)

![Java](https://img.shields.io/badge/Java-8+-orange)
![C++](https://img.shields.io/badge/C++-20-blue)
![JNI](https://img.shields.io/badge/JNI-Native-green)
![CMake](https://img.shields.io/badge/CMake-Build-red)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)

![JVMTI](https://img.shields.io/badge/JVMTI-Agent-success)
![Injection](https://img.shields.io/badge/Runtime-Injection-blueviolet)
![Bytecode](https://img.shields.io/badge/Bytecode-Transform-blue)
![Attach](https://img.shields.io/badge/DisableAttach-Bypass-important)

## 目录

- [JuiceAgent](#juiceagent)
  - [简介](#简介)
  - [工作原理](#工作原理)
  - [特性](#特性)
  - [项目结构](#项目结构)
  - [快速开始](#快速开始)
  - [JuiceAgent API](#juiceagent-api)
  - [示例](#示例)
  - [如何构建](#如何构建)
  - [完整免责声明](#完整免责声明)
  - [致谢](#致谢)

## 简介

JuiceAgent 是 [JuiceAgent-API](https://github.com/xiaozhou233/JuiceAgent-API) 的原生实现。它可以将外部 JAR 加载到正在运行的 JVM 中、调用指定的入口类/方法，并重新定义或重变换已加载的类——全程无需 `-javaagent` 启动参数，即使 `DisableAttachMechanism=true` 也能工作。

使用 JuiceAgent 可以做到：

- 将外部 JAR 动态加载到正在运行的 JVM 进程
- 注入后调用指定的入口类和入口方法
- 在运行时重新定义已加载的类
- 重变换字节码以进行插桩或修改
- 查询已加载的类并读取类的字节码

**如果你有任何问题或建议，欢迎提 issue 或提交 pull request。=D**

## 工作原理

1. `injector.exe`（或通过 JNI 调用 `libinject.dll`）将 `libloader.dll` 反射注入到目标 JVM 进程。
2. `libloader.dll` 附加到 JVM，读取 `config.toml`，解出内嵌的 `JuiceAgent-API` jar 并将其加入系统类加载器的搜索路径。
3. 加载器引导 agent：加载 `libagent.dll` 并初始化 JVMTI agent（`JuiceAgent::Agent`）。
4. agent 注册 JVMTI 回调并启动配置的模块（例如 `JarLoader`），加载目标 JAR 并调用入口类/方法。

`JuiceAgent-API` jar 内嵌于原生二进制中，无需单独下载。

## 特性

- 将外部 JAR 加载到 Bootstrap 或 System ClassLoader
- 将 JAR 添加到任意类加载器
- 动态定义类
- 按名称或类引用重新定义类
- 按名称或类引用重变换类
- 获取所有已加载的类
- 按名称查找类
- 按名称或类引用获取类的字节码
- 自动附加 JVM，支持 `DisableAttachMechanism=true`

## 项目结构

- `src/libloader` — `libloader.dll`。注入到目标 JVM，附加 JVM、解析 `config.toml`、注入内嵌的 `JuiceAgent-API` 并引导 agent。
- `src/libagent` — `libagent.dll`。JVMTI agent。注册回调（例如 `ClassFileLoadHook`）并实现 `JuiceAgent-API` 的原生侧。
- `src/libinjector` — `injector.exe` + `libinject.dll`。反射式 DLL 注入器，将 `libloader.dll` 加载到目标进程。可通过命令行或 JNI 驱动。

## 快速开始

### 1. 下载 Release

从 [releases](https://github.com/xiaozhou233/JuiceAgent/releases) 页面下载 `JuiceAgent_vx.x.x_x64.zip`。

解压 ZIP 到一个目录：

- YourDir
  - libagent.dll
  - libinject.dll
  - libloader.dll
  - injector.exe

### 2. 拷贝自定义 JAR / 依赖

- YourDir
  - libagent.dll
  - libinject.dll
  - libloader.dll
  - injector.exe
  - **MyCustomJar.jar**
  - **injection**
    - **Dependencies1.jar**
    - **Dependencies2.jar**

### 3. 编写配置

```toml
[JuiceAgent]
Version = 1

[JuiceAgent.Loader]
# libagent.dll 的路径，默认是 "./libagent.dll"
JuiceAgentNativeLibraryPath = ""

[JuiceAgent.Modules]

[JuiceAgent.Modules.JarLoader]
Enabled = true
# 需要优先加载的依赖 JAR 所在目录
InjectionDir = "./injection"
# 要加载的 JAR 文件路径
JarPath = "./MyCustomJar.jar"
# 加载 JAR 后要执行的入口类
EntryClass = "Example.Main"
# 入口方法，必须是：public static void run()
EntryMethod = "run"
```

将文件保存为 `config.toml` 并放在同一目录（运行时目录）中。相对路径以运行时目录为基准解析。

- YourDir
  - **config.toml**
  - libagent.dll
  - libinject.dll
  - libloader.dll
  - injector.exe
  - MyCustomJar.jar
  - injection
    - Dependencies1.jar
    - Dependencies2.jar

### 4. 运行注入器

#### 方法 A：使用 `injector.exe`

在 `YourDir` 下运行 `injector.exe`：

```text
<jps output>
Input PID:
```

输入目标 JVM 进程的 PID 并按回车。

也可以直接传参：`injector.exe <pid> <libloader.dll 路径> [libinject.dll 路径]`。

#### 方法 B：通过 JNI 调用 `inject`

详见 [docs/Inject.md](docs/Inject.md)。

### 5. 完成

目标 JVM 将加载指定的 JAR 并执行指定的入口类和入口方法。

## JuiceAgent API

以下原生函数通过 `cn.xiaozhou233.juiceagent.api.JuiceAgent` 类暴露给 Java（Java API 定义见 [JuiceAgent-API](https://github.com/xiaozhou233/JuiceAgent-API) 项目）：

- `init(String runtimeDir)` — 从 Java 初始化 agent
- `addToBootstrapClassLoaderSearch(String jarPath)`
- `addToSystemClassLoaderSearch(String jarPath)`
- `addToClassLoader(String jarPath, ClassLoader targetClassLoader)`
- `defineClass(ClassLoader targetClassLoader, byte[] bytes)` — 返回 `Class<?>`
- `redefineClass(Class<?> clazz, byte[] classBytes, int length)`
- `redefineClassByName(String className, byte[] classBytes, int length)`
- `retransformClass(Class<?> clazz, byte[] bytecodes, int length)`
- `retransformClassByName(String className, byte[] bytecodes, int length)`
- `getLoadedClasses()` — 返回 `Class<?>[]`
- `getClassByName(String name)` — 返回 `Class<?>`
- `getClassBytes(Class<?> clazz)` — 返回 `byte[]`
- `getClassBytesByName(String name)` — 返回 `byte[]`

## 示例

- `examples/load-via-injector-exe` — 通过 `injector.exe` 将 JAR 加载到目标 JVM 并运行入口点。
- `examples/retansform-class` — 演示使用目标 JVM 和修改后的类副本进行类重变换。

## 如何构建

### 环境要求

- Java 8+
- MinGW
- CMake 3.15+
- Ninja

### 使用 CMake Presets 构建

```powershell
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

cmake --preset release
cmake --build build
```

### 手动构建

```powershell
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

生成的二进制文件将放在 `build/bin` 中。

## 完整免责声明

本项目仅用于受控环境下的学习与研究。作者不对因未授权使用、修改、破解或违反任何适用协议或法律所造成的后果负责。用户有责任确保其行为合法合规。

## 致谢

- [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) — DLL 注入实现
- [plog](https://github.com/SergiusTheBest/plog) — 轻量、简单、可扩展的 C++ 日志库
- [toml11](https://github.com/ToruNiina/toml11) — 现代 C++ 的 TOML 库
- [eventpp](https://github.com/wqking/eventpp) — 事件分发器与回调列表 C++ 库
