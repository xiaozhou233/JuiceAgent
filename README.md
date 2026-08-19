# JuiceAgent

[English](README.md) | [简体中文](README_ZH.md)

JuiceAgent is an advanced JVMTI-based injection framework for runtime JAR loading, bytecode transformation, and runtime JVM instrumentation. It injects into a running JVM process even when `DisableAttachMechanism=true`, without requiring `-javaagent` startup arguments or the attach mechanism.

**Warning**: This project is still experimental and is not recommended for production use. Use it at your own risk.

> **Tips**: Some antivirus software may flag JuiceAgent as malware. See [Antivirus False Positive Notice](docs/Antivirus-False-Positive.md) for details.

**Note**: This project is designed for a regular JVM. Custom JVMs may not be able to run it.

Welcome to join our QQ/Discord group for more information and support:
- Discord: [JuiceDev](https://discord.gg/AQTnUyWNNJ)
- QQ Group: 1103504356


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

## Index

- [JuiceAgent](#juiceagent)
  - [Introduction](#introduction)
  - [How It Works](#how-it-works)
  - [Features](#features)
  - [Project Structure](#project-structure)
  - [Quick Start](#quick-start)
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

- `src/libloader` — `libloader.dll`. Injected into the target JVM. Attaches to the JVM, parses `config.toml`, injects the embedded `JuiceAgent-API`, and bootstraps the agent.
- `src/libagent` — `libagent.dll`. The JVMTI agent. Registers callbacks (e.g. `ClassFileLoadHook`) and implements the native side of `JuiceAgent-API`.
- `src/injector` — `injector.exe`. Command-line driver that loads `libinject.dll` and injects `libloader.dll` into the target process.
- `src/libinject` — `libinject.dll`. Reflective DLL injector. Exposes a native `inject` function and JNI bindings (`cn.xiaozhou233.juiceagent.injector.InjectorNative`).

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
Version = 1

[JuiceAgent.Loader]
# Path to libagent.dll, default is "./libagent.dll"
JuiceAgentNativeLibraryPath = ""

[JuiceAgent.Modules]

[JuiceAgent.Modules.JarLoader]
Enabled = true
# Directory containing dependency JARs to load first
InjectionDir = "./injection"
# Path to the JAR file to be loaded
JarPath = "./MyCustomJar.jar"
# Entry class to be executed after loading the JAR
EntryClass = "Example.Main"
# Entry method, must be: public static void run()
EntryMethod = "run"
```

Save the file as `config.toml` in the same directory (the runtime directory). Relative paths are resolved against the runtime directory.

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

See [Inject](doc/Inject.md)

### 5. Done

The target JVM will load the specified JAR and execute the specified entry class and method.

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

- `examples/load-via-injector-exe` — loads a JAR into a target JVM via `injector.exe` and runs an entry point.
- `examples/retransform-class` — demonstrates class retransformation with a target JVM and a modified copy of the class.

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

The generated binaries will be placed in `build/bin`.

## Full Disclaimer

This project is intended for learning and research in controlled environments only. The author is not responsible for any consequences arising from unauthorized use, modification, cracking, or violation of any applicable agreements or laws. Users are responsible for ensuring that their actions are legal and compliant.

## Acknowledgements

- [ReflectiveDLLInjection](https://github.com/stephenfewer/ReflectiveDLLInjection) — DLL injection implementation
- [spdlog](https://github.com/gabime/spdlog) — Fast C++ logging library
- [toml11](https://github.com/ToruNiina/toml11) — TOML for Modern C++
