# JuiceAgent

[English](README.md) | [简体中文](README_ZH.md)

JuiceAgent is an advanced JVMTI-based injection framework for runtime JAR loading, bytecode transformation, and runtime JVM instrumentation. It injects into a running JVM process even when `DisableAttachMechanism=true`, without requiring `-javaagent` startup arguments or the attach mechanism.

Two platforms are supported:

- **Windows x64** — reflective DLL injection (`injector.exe` + `libinject.dll`).
- **Linux x64** — `ptrace` attach plus a remote `dlopen` call that makes the target JVM load `libloader.so` itself.

**Warning**: This project is still experimental and is not recommended for production use. Use it at your own risk.

> **Tips**: Some antivirus software may flag JuiceAgent as malware. See [Antivirus False Positive Notice](docs/Antivirus-False-Positive.md) for details.

**Note**: This project is designed for a regular JVM. Custom JVMs may not be able to run it.

Welcome to join our QQ/Discord group for more information and support:
- Discord: [JuiceDev](https://discord.gg/AQTnUyWNNJ)
- QQ Group: 1103504356

> **New to JuiceAgent?** You do not have to wire up the injection by hand. A
> coding agent such as OpenCode can do it for you: open this repository in the
> agent and ask it to read [`QuickStartForAgents.md`](QuickStartForAgents.md) and
> follow it. That guide covers the whole process — environment check, requirement
> gathering, `config.toml`, injection and verification — and is written to be
> executed by an agent. Simply tell the agent:
> "Read `QuickStartForAgents.md` and follow it to inject my JAR."

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

## Index

- [JuiceAgent](#juiceagent)
  - [Introduction](#introduction)
  - [How It Works](#how-it-works)
  - [Features](#features)
  - [Project Structure](#project-structure)
  - [Quick Start](#quick-start)
  - [Linux Support](#linux-support)
  - [JuiceAgent API](#juiceagent-api)
  - [Examples](#examples)
  - [How To Build](#how-to-build)
  - [Full Disclaimer](#full-disclaimer)
  - [Acknowledgements](#acknowledgements)

## Introduction

JuiceAgent is the native implementation of the [JuiceAgent-API](https://github.com/xiaozhou233/JuiceAgent-API). It loads external JARs into a running JVM, invokes a specified entry class/method, and redefines or retransforms already-loaded classes — all without a `-javaagent` startup flag and even when `DisableAttachMechanism=true`.

With JuiceAgent, developers can:

- Dynamically load external JARs into a running JVM process
- Invoke a specified entry class and method after injection
- Redefine already loaded classes at runtime
- Retransform bytecode for instrumentation or modification
- Query loaded classes and read class bytecode

**If you have any questions or suggestions, please feel free to open an issue or submit a pull request. =D**

## How It Works

1. `injector.exe` (or `libinject.dll` via JNI) reflectively injects `libloader.dll` into the target JVM process.
2. `libloader.dll` attaches to the JVM, reads `config.toml`, extracts the embedded `JuiceAgent-API` jar, and adds it to the system class loader search path.
3. The loader bootstraps the agent: it loads `libagent.dll` and initializes the JVMTI agent (`JuiceAgent::Agent`).
4. The agent registers JVMTI callbacks and starts the configured modules (e.g. `JarLoader`), which loads the target JAR and invokes the entry class/method.

The `JuiceAgent-API` jar is embedded in the native binaries, so no separate download is required.

On Linux the pipeline is the same, only the injection step changes. The injector
attaches to the target process with `ptrace`, saves its registers, and performs a
remote `dlopen` so the JVM loads `libloader.so` in-process (no `LD_PRELOAD`, no
`-javaagent`). `libloader.so` then starts from an ELF constructor, resolves its
runtime directory (`JuiceAgent_ConfigPath`, or `$HOME/.juiceagent` by default),
and drives the agent exactly like the Windows loader. See
[Linux Support](#linux-support).

## Features

- Load external JARs into the Bootstrap or System ClassLoader
- Add JARs to an arbitrary class loader
- Define classes dynamically
- Redefine classes by name or class reference
- Retransform classes by name or class reference
- Get all loaded classes
- Find a class by name
- Get class bytecode by name or class reference
- Automatic JVM attachment that works with `DisableAttachMechanism=true`

## Project Structure

- `src/libloader` — `libloader.dll` (Windows) / `libloader.so` (Linux). Injected into the target JVM. Attaches to the JVM, parses `config.toml`, injects the embedded `JuiceAgent-API`, and bootstraps the agent.
- `src/libagent` — `libagent.dll` (Windows) / `libagent.so` (Linux). The JVMTI agent. Registers callbacks (e.g. `ClassFileLoadHook`) and implements the native side of `JuiceAgent-API`.
- `src/injector` — `injector.exe` (Windows) / `injector` (Linux). Command-line driver that loads the loader into the target process.
- `src/libinject` — `libinject.dll` (Windows only). Reflective DLL injector. Exposes a native `inject` function and JNI bindings (`cn.xiaozhou233.juiceagent.injector.InjectorNative`).

## Quick Start

**You can also see the [examples](https://github.com/xiaozhou233/JuiceAgent/tree/main/examples) directory for more detailed usage examples.**

### 1. Download Release

Download `JuiceAgent_vx.x.x_x64.zip` from the [releases](https://github.com/xiaozhou233/JuiceAgent/releases) page.

Extract the ZIP file to a directory:

- YourDir
  - libagent.dll
  - libinject.dll
  - libloader.dll
  - injector.exe

### 2. Copy your Custom JAR / Dependencies

- YourDir
  - libagent.dll
  - libinject.dll
  - libloader.dll
  - injector.exe
  - **MyCustomJar.jar**
  - **injection**
    - **Dependencies1.jar**
    - **Dependencies2.jar**

### 3. Write Config

```toml
[JuiceAgent]
Version = 2
# Path to libagent.dll, default is "./libagent.dll" (relative to the runtime directory)
JuiceAgentNativeLibraryPath = ""

[JarLoader]
Enabled = true
# Mode: BuiltIn | CustomJar
Mode = "BuiltIn"
# Directory containing dependency JARs to load first (BuiltIn mode)
InjectionDir = "./injection"
# Path to the JAR file to be loaded
JarPath = "./MyCustomJar.jar"
# Entry class to be executed after loading the JAR
EntryClass = "Example.Main"
# Entry method, must be: public static void run()
EntryMethod = "run"
```

Save the file as `config.toml` in the same directory (the runtime directory). Relative paths are resolved against the runtime directory. See [Configuration](docs/config.md) for the complete reference.

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

### 4. Run the Injector

#### Method A: Use `injector.exe`

Run `injector.exe` from `YourDir`:

```text
<jps output>
Input PID:
```

Enter the PID of the target JVM process and press Enter.

You can also pass arguments directly: `injector.exe <pid> <libloader.dll path> [libinject.dll path]`.

#### Method B: Use JNI to call `inject`

See [Inject](docs/Inject.md)

### 5. Done

The target JVM will load the specified JAR and execute the specified entry class and method.

## Linux Support

JuiceAgent also works on Linux x64. Instead of reflective DLL injection, the
Linux injector uses `ptrace` to attach to the target process and then performs a
**remote `dlopen`**: it saves the thread state, makes the target call
`dlopen("libloader.so", RTLD_NOW)`, restores the state and detaches. The JVM
therefore loads the loader itself — no `LD_PRELOAD` and no `-javaagent` — and the
rest of the pipeline is identical to Windows.

### Components

- `libloader.so` — loaded into the target JVM, starts from an ELF constructor.
- `libagent.so` — the JVMTI agent. This is the default
  `JuiceAgentNativeLibraryPath` on Linux (instead of `libagent.dll`).
- `injector` — attaches to the target PID and performs the remote `dlopen`.

### Configuration location

On Linux the loader does **not** look in the current working directory. It
resolves the runtime directory in this order:

1. `JuiceAgent_ConfigPath` — if the environment variable is set and non-empty, it
   is used as-is.
2. `$HOME/.juiceagent` — the default; created automatically if it does not exist.

`config.toml` is read from that directory. The schema is identical to Windows
and relative paths (`JarPath`, `InjectionDir`, ...) are resolved against it, so a
typical layout is:

```text
~/.juiceagent/
├── config.toml
├── MyCustomJar.jar
└── injection/
    └── Dependencies.jar
```

To point the loader at another directory:

```bash
export JuiceAgent_ConfigPath=/opt/juiceagent
```

### Requirements

- Run the injector as the same user as the target JVM (or as `root`), and make
  sure `ptrace` is permitted. If injection fails with `EPERM`, check
  `/proc/sys/kernel/yama/ptrace_scope`.
- The target JVM and the loader must both be x64.

> **Note**: The Linux injector binary is not open source yet. The `injector`
> target in this repository is a placeholder that only prints a notice; the
> loader and the agent are fully open source and build from this tree.

## JuiceAgent API

The following native functions are exposed to Java through the `cn.xiaozhou233.juiceagent.api.JuiceAgent` class (see the [JuiceAgent-API](https://github.com/xiaozhou233/JuiceAgent-API) project for the Java API definition):

- `init(String runtimeDir)` — initialize the agent from Java
- `addToBootstrapClassLoaderSearch(String jarPath)`
- `addToSystemClassLoaderSearch(String jarPath)`
- `addToClassLoader(String jarPath, ClassLoader targetClassLoader)`
- `defineClass(ClassLoader targetClassLoader, byte[] bytes)` — returns `Class<?>`
- `redefineClass(Class<?> clazz, byte[] classBytes, int length)`
- `redefineClassByName(String className, byte[] classBytes, int length)`
- `retransformClass(Class<?> clazz, byte[] bytecodes, int length)`
- `retransformClassByName(String className, byte[] bytecodes, int length)`
- `getLoadedClasses()` — returns `Class<?>[]`
- `getClassByName(String name)` — returns `Class<?>`
- `getClassBytes(Class<?> clazz)` — returns `byte[]`
- `getClassBytesByName(String name)` — returns `byte[]`

## Examples

- `examples/inject-via-java-api` — injects JuiceAgent into a running JVM through the Java API (JNI), without needing `injector.exe`.
- `examples/load-via-injector-exe` — loads a JAR into a target JVM via `injector.exe` and runs an entry point.
- `examples/retransform-class` — demonstrates class retransformation with a target JVM and a modified copy of the class.

## How To Build

### Requirements

- Java 8+
- MinGW-w64 (Windows) or GCC/Clang (Linux)
- CMake 3.15+
- Ninja

### Build with CMake Presets

```powershell
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

cmake --preset release
cmake --build build
```

### Manual Build

```powershell
git clone https://github.com/xiaozhou233/JuiceAgent.git
cd JuiceAgent

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The generated binaries will be placed in `build/bin`. On Linux this produces
`libloader.so`, `libagent.so` and `injector`; on Windows the `.dll` / `.exe`
equivalents.

## Full Disclaimer

This project is intended for learning and research in controlled environments only. The author is not responsible for any consequences arising from unauthorized use, modification, cracking, or violation of any applicable agreements or laws. Users are responsible for ensuring that their actions are legal and compliant.

## Acknowledgements

- [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) — DLL injection implementation
- [spdlog](https://github.com/gabime/spdlog) — Fast C++ logging library
- [toml11](https://github.com/ToruNiina/toml11) — TOML for Modern C++
