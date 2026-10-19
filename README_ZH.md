# JuiceAgent

[English](README.md) | [简体中文](README_ZH.md)

> **注意**：本文档由 AI 自动翻译，可能存在不准确或不通顺之处，请以 [英文版 README](README.md) 为准。如有疑问欢迎提 issue 或 PR 修正。

JuiceAgent 是一个基于 JVMTI 的高级注入框架，用于运行时 JAR 加载、字节码变换和运行时 JVM 插桩。它可以在 `DisableAttachMechanism=true` 的情况下注入到正在运行的 JVM 进程中，无需 `-javaagent` 启动参数，也无需使用 attach 机制。

目前支持两个平台：

- **Windows x64**——反射式 DLL 注入（`injector.exe` + `libinject.dll`）。
- **Linux x64**——通过 `ptrace` 附加目标进程，再用一次远程 `dlopen` 让目标 JVM 自己加载 `libloader.so`。

**警告**：本项目仍处于实验阶段，不建议用于生产环境。使用风险自负。

> **提示**：部分杀毒软件可能将 JuiceAgent 误报为恶意软件。详情见[杀毒软件误报说明](docs/Antivirus-False-Positive.md)。

**注意**：本项目针对标准 JVM 设计，自定义 JVM 可能无法运行。

欢迎加入我们的 QQ / Discord 群组获取更多信息和支持：
- Discord：[JuiceDev](https://discord.gg/AQTnUyWNNJ)
- QQ 群：1103504356

> **不知道该怎么用？** 不必手动折腾注入流程：用支持读写文件和执行命令的 AI Agent（例如 OpenCode）即可。在本仓库中打开 Agent，让它先阅读 [`QuickStartForAgents.md`](QuickStartForAgents.md) 并照着执行——该文档覆盖了完整流程（环境检查、需求确认、`config.toml`、注入与验证），并且是专门写给 Agent 执行的。你只需要告诉它：
> "请阅读 `QuickStartForAgents.md`，并照着里面的步骤把我的 JAR 注入到目标 JVM。"

![License](https://img.shields.io/github/license/xiaozhou233/JuiceAgent)
![Release](https://img.shields.io/github/v/release/xiaozhou233/JuiceAgent)
![GitHub last commit](https://img.shields.io/github/last-commit/xiaozhou233/JuiceAgent)
![GitHub repo size](https://img.shields.io/github/repo-size/xiaozhou233/JuiceAgent)
![Downloads](https://img.shields.io/github/downloads/xiaozhou233/JuiceAgent/total)

![Java](https://img.shields.io/badge/Java-8+-orange)
![C++](https://img.shields.io/badge/C++-20-blue)
![JNI](https://img.shields.io/badge/JNI-Native-green)
![CMake](https://img.shields.io/badge/CMake-Build-red)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey)

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
  - [Linux 支持](#linux-支持)
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

Linux 上的流程与此相同，只有注入这一步不同。注入器通过 `ptrace` 附加到目标进程，保存线程状态，发起一次远程 `dlopen` 让 JVM 在自身进程内加载 `libloader.so`（无需 `LD_PRELOAD`，也无需 `-javaagent`）。随后 `libloader.so` 从 ELF constructor 启动，解析运行时目录（`JuiceAgent_ConfigPath`，默认 `$HOME/.juiceagent`），之后与 Windows 加载器完全一致地驱动 agent。详见 [Linux 支持](#linux-支持)。

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

- `src/libloader` — `libloader.dll`（Windows）/ `libloader.so`（Linux）。注入到目标 JVM，附加 JVM、解析 `config.toml`、注入内嵌的 `JuiceAgent-API` 并引导 agent。
- `src/libagent` — `libagent.dll`（Windows）/ `libagent.so`（Linux）。JVMTI agent。注册回调（例如 `ClassFileLoadHook`）并实现 `JuiceAgent-API` 的原生侧。
- `src/injector` — `injector.exe`（Windows）/ `injector`（Linux）。命令行驱动，将加载器注入到目标进程。
- `src/libinject` — `libinject.dll`（仅 Windows）。反射式 DLL 注入器，导出原生 `inject` 函数并提供 JNI 绑定（`cn.xiaozhou233.juiceagent.injector.InjectorNative`）。

## 快速开始

**你也可以查看 [examples](https://github.com/xiaozhou233/JuiceAgent/tree/main/examples) 目录获取更详细的使用示例。**

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
Version = 2
# libagent.dll 的路径，默认是 "./libagent.dll"（相对于运行时目录）
JuiceAgentNativeLibraryPath = ""

[JarLoader]
Enabled = true
# Mode：BuiltIn | CustomJar
Mode = "BuiltIn"
# 需要优先加载的依赖 JAR 所在目录（BuiltIn 模式）
InjectionDir = "./injection"
# 要加载的 JAR 文件路径
JarPath = "./MyCustomJar.jar"
# 加载 JAR 后要执行的入口类
EntryClass = "Example.Main"
# 入口方法，必须是：public static void run()
EntryMethod = "run"
```

将文件保存为 `config.toml` 并放在同一目录（运行时目录）中。相对路径以运行时目录为基准解析。完整配置说明见 [Configuration](docs/config.md)。

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

看 [Inject](docs/Inject.md)

### 5. 完成

目标 JVM 将加载指定的 JAR 并执行指定的入口类和入口方法。

## Linux 支持

JuiceAgent 同样支持 Linux x64。与反射式 DLL 注入不同，Linux 注入器使用 `ptrace` 附加目标进程，然后执行一次**远程 `dlopen`**：保存线程状态，让目标进程调用 `dlopen("libloader.so", RTLD_NOW)`，恢复状态并 detach。这样由 JVM 自己加载加载器——无需 `LD_PRELOAD`，也无需 `-javaagent`——其后的流程与 Windows 完全一致。

### 组件

- `libloader.so` —— 加载进目标 JVM，从 ELF constructor 启动。
- `libagent.so` —— JVMTI agent。在 Linux 上它是 `JuiceAgentNativeLibraryPath` 的默认值（而不是 `libagent.dll`）。
- `injector` —— 附加到目标 PID 并执行远程 `dlopen`。

### 配置位置

Linux 上加载器**不会**在当前工作目录中查找配置，而是按以下顺序确定运行时目录：

1. `JuiceAgent_ConfigPath` —— 环境变量存在且非空时直接使用。
2. `$HOME/.juiceagent` —— 默认值，目录不存在时会自动创建。

`config.toml` 从该目录读取，配置结构（schema）与 Windows 完全一致，相对路径（`JarPath`、`InjectionDir` 等）也以该目录为基准解析。典型目录结构：

```text
~/.juiceagent/
├── config.toml
├── MyCustomJar.jar
└── injection/
    └── Dependencies.jar
```

如需指定其他目录：

```bash
export JuiceAgent_ConfigPath=/opt/juiceagent
```

### 环境要求

- 以与目标 JVM 相同的用户（或 `root`）运行注入器，并确保允许 `ptrace`。如果注入失败并返回 `EPERM`，请检查 `/proc/sys/kernel/yama/ptrace_scope`。
- 目标 JVM 与加载器都必须是 x64。

> **注意**：Linux 注入器二进制目前尚未开源。本仓库中的 `injector` 目标只是一个占位程序，仅打印提示信息；加载器与 agent 完全开源，可直接从本仓库构建。

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

- `examples/inject-via-java-api` — 通过 Java API（JNI）将 JuiceAgent 注入到正在运行的 JVM，无需 `injector.exe`。
- `examples/load-via-injector-exe` — 通过 `injector.exe` 将 JAR 加载到目标 JVM 并运行入口点。
- `examples/retransform-class` — 演示使用目标 JVM 和修改后的类副本进行类重变换。

## 如何构建

### 环境要求

- Java 8+
- MinGW-w64（Windows）或 GCC/Clang（Linux）
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

生成的二进制文件将放在 `build/bin` 中。Linux 下会生成 `libloader.so`、`libagent.so` 和 `injector`；Windows 下为对应的 `.dll` / `.exe`。

## 完整免责声明

本项目仅用于受控环境下的学习与研究。作者不对因未授权使用、修改、破解或违反任何适用协议或法律所造成的后果负责。用户有责任确保其行为合法合规。

## 致谢

- [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) — DLL 注入实现
- [spdlog](https://github.com/gabime/spdlog) — 高性能 C++ 日志库
- [toml11](https://github.com/ToruNiina/toml11) — 现代 C++ 的 TOML 库
