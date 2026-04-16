# JuiceAgent
[English](https://github.com/xiaozhou233/JuiceAgent/blob/main/README.md) | [中文](https://github.com/xiaozhou233/JuiceAgent/blob/main/README_zh.md)

JuiceAgent —— 一个基于 JVMTI 的注入库，用于加载外部 JAR 并对 Java 字节码进行热替换，即使在 DisableAttachMechanism 启用的情况下也可使用。

**注意：** 当前仅支持 **Windows**，未来计划支持 **Linux**。

**注意：** **Version 3.1 Build 1 +** 是当前的 **稳定版本**。3.0 及之后版本已重构，可能存在未知问题。

**警告：** 本项目仍处于实验阶段，不建议用于生产环境。  
**使用风险自负。**

---

## 简介
JuiceAgent 是一个基于 JVMTI 的原生注入框架，通过向目标 JVM 注入代码，实现动态加载外部 JAR 以及字节码热替换，无需使用 javaagent（在 DisableAttachMechanism=true 时仍可工作）。

---

## 功能特性

本项目是 JuiceAgent-API 的Native实现，详细内容请参考：  
https://github.com/xiaozhou233/JuiceAgent-API/blob/master/src/main/java/cn/xiaozhou233/juiceagent/api/JuiceAgent.java

- addToBootstrapClassLoaderSearch
- addToSystemClassLoaderSearch
- addToClassLoader
- defineClass
- redefineClass(ByName)
- retransformClass(ByName)
- getLoadedClasses
- getClassByName
- getClassBytes(ByName)

---

## 使用方法（适用于 Version 3.0 Build 2）

### 1. ⬇️ 下载文件

- 从 Release 下载：
  - libagent.dll
  - libinject.dll
  - libloader.dll
  - injector.exe  
  并放入 `YourDir`

- 从 JuiceAgent-API Release 下载：
  - JuiceAgent-API-X.X.X+build.X.jar  
  并放入 `YourDir`

---

### 2. ✍️ 编写配置文件

在 `YourDir` 下创建 `config.toml`

```toml
[JuiceAgent]
# 默认值: YourDir/JuiceAgent-API.jar
JuiceAgentAPIJarPath = ""
# 默认值: YourDir/libagent.dll
JuiceAgentNativePath = ""

[Entry]
# 默认值: YourDir/Entry.jar
EntryJarPath = ""
# 默认值: com.example.Entry
EntryClass = ""
# 默认值: start
EntryMethod = "start"

[Runtime]
# 默认值: YourDir/injection
InjectionDir = ""
```

---

### 3. 💉 执行注入

#### 方法 A：使用 injector.exe

运行`inject.exe`：

```
114514 loop
Input PID:
```

输入目标进程 PID 后回车

---

#### 方法 B：使用 JNI 调用 inject

创建文件：  
`cn/xiaozhou233/juiceagent/injector/InjectorNative.java`

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

调用示例：

```java
import cn.xiaozhou233.juiceagent.injector.InjectorNative;

System.load("<path-to-libinject>");
InjectorNative injectorNative = new InjectorNative();

injectorNative.inject(<pid>, "<path-to-libloader>", "<path-to-toml-config-dir>");
```

---

### 4. ✅ 完成

注入成功后，目标进程将加载：

- Entry.jar（或配置中的 EntryJarPath）
- 并执行 EntryClass.EntryMethod（默认 com.example.Entry.start() ）

---

## 如何构建

### 环境要求

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

生成的 DLL 位于：

```
build/bin
```

JAR 可在 JuiceAgent-API 项目中构建或下载。

---

## 完整免责声明

免责声明：本项目仅建议在受控环境中用于学习与研究。作者不对任何因滥用、破解、修改或违反协议/法律行为所产生的后果负责。使用者需自行确保合法合规，并承担全部责任。

---

## 致谢

- ReflectiveDLLInjection — DLL 注入实现  
- plog — 轻量级 C++ 日志库  
- tinytoml — C++11 TOML 解析库  
- [eventpp](https://github.com/wqking/eventpp) - 用于事件分发器和回调列表的 C++ 库
