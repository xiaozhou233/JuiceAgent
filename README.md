# JuiceAgent

JuiceAgent is an advanced JVMTI-based injection framework designed for runtime JAR loading, bytecode transformation, and runtime JVM instrumentation, even when DisableAttachMechanism is enabled.

**Warning**: This project is still experimental and is not recommended for production use. Use it at your own risk.

![License](https://img.shields.io/github/license/xiaozhou233/JuiceAgent)
![Release](https://img.shields.io/github/v/release/xiaozhou233/JuiceAgent)
![GitHub last commit](https://img.shields.io/github/last-commit/xiaozhou233/JuiceAgent)
![GitHub repo size](https://img.shields.io/github/repo-size/xiaozhou233/JuiceAgent)

![Java](https://img.shields.io/badge/Java-8+-orange)
![C++](https://img.shields.io/badge/C++-20-blue)
![JNI](https://img.shields.io/badge/JNI-Native-green)
![CMake](https://img.shields.io/badge/CMake-Build-red)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)

![JVMTI](https://img.shields.io/badge/JVMTI-Agent-success)
![Injection](https://img.shields.io/badge/Runtime-Injection-blueviolet)
![Bytecode](https://img.shields.io/badge/Bytecode-Transform-blue)
![Attach](https://img.shields.io/badge/DisableAttach-Bypass-important)

## Introduction
JuiceAgent is a JVMTI-native injection framework for loading external JARs and redefining or retransformation of Java classes in a target JVM without requiring -javaagent startup arguments or attach javaagent. It also works when DisableAttachMechanism=true.

With JuiceAgent, developers can:
 - Dynamically load external JARs into a running JVM process
 - Invoke a specified entry class and method after injection    
 - Redefine already loaded classes at runtime
 - Retransform bytecode for instrumentation or modification
 - And more...

**If you have any questions or suggestions, please feel free to open an issue or submit a pull request. =D**

## Index
- [JuiceAgent](#juiceagent)
  - [Introduction](#introduction)
  - [Index](#index)
  - [Features](#features)
  - [Quick Start](#quick-start)
    - [1. Download Release](#1-download-release)
    - [2. Copy your custom JAR/Dependencies to Directory](#2-copy-your-custom-jardependencies-to-directory)
    - [3. Write Config](#3-write-config)
    - [4. Run the Injector](#4-run-the-injector)
      - [Method A: Use `injector.exe`](#method-a-use-injectorexe)
      - [Method B: Use JNI to call `inject`](#method-b-use-jni-to-call-inject)
    - [5. Done](#5-done)
  - [How To Build](#how-to-build)
    - [Requirements](#requirements)
    - [Build with CMake Presets](#build-with-cmake-presets)
    - [Manual Build](#manual-build)
  - [Full Disclaimer](#full-disclaimer)
  - [Acknowledgements](#acknowledgements)

## Features
This project is the native implementation of JuiceAgent-API. See [JuiceAgent.java](https://github.com/xiaozhou233/JuiceAgent-API/blob/master/src/main/java/cn/xiaozhou233/juiceagent/api/JuiceAgent.java) for the API definition.

- Load JARs into the Bootstrap or System ClassLoader
- Define classes dynamically
- Redefine classes by name or class reference
- Retransform classes by name or class reference
- Get all loaded classes
- Find classes by name
- Get class bytecode by name or class reference

## Quick Start
### 1. Download Release
Download `libagent.dll` `libinject.dll` `libloader.dll` and `injector.exe` from the [releases](https://github.com/xiaozhou233/JuiceAgent/releases) page.

Download `JuiceAgent-API-x.x.x+build.x.jar` from the [JuiceAgent-API](https://github.com/xiaozhou233/JuiceAgent-API/releases) page.

Put the downloaded files into the same directory.
- YourDir
  - JuiceAgent-API-x.x.x+build.x.jar
  - libagent.dll
  - libinject.dll
  - libloader.dll
  - injector.exe

### 2. Copy your custom JAR/Dependencies to Directory
- YourDir
  - JuiceAgent-API-x.x.x+build.x.jar
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
# Path to JuiceAgent-API-x.x.x+build.x.jar
JuiceAgentAPIJarPath = "./JuiceAgent-API-x.x.x+build.x.jar"
# Path to libagent.dll, default is "./libagent.dll"
JuiceAgentNativeLibraryPath = ""

[JuiceAgent.Modules]

[JuiceAgent.Modules.JarLoader]
Enabled = true
# Path to the Injection JAR files to be loaded
InjectionDir = "./injection"
# Path to the JAR file to be loaded
JarPath = "./MyCustomJar.jar"
# Entry class to be executed after loading the JAR
EntryClass = "Example.Main"
# Entry method to be executed after loading the JAR
EntryMethod = "run"
```

Save the config file as `config.toml`.

- YourDir
  - **config.toml**
  - JuiceAgent-API-x.x.x+build.x.jar
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
Run `injector.exe` from `YourDir`.

```text
<jps output>
Input PID:
```

Enter the PID of the target JVM process and press Enter.

#### Method B: Use JNI to call `inject`

See [Documents](https://github.com/xiaozhou233/JuiceAgent/blob/master/docs/Inject.md) for details.

### 5. Done
The target JVM will load the specified JAR and execute the specified entry class and method.

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
- [toml11](https://github.com/ToruNiina/toml11) - TOML for Modern C++
- [eventpp](https://github.com/wqking/eventpp) - C++ library for event dispatchers and callback lists
