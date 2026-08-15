# Examples

JuiceAgent 示例集合，演示将 JuiceAgent 注入到正在运行的 JVM 进程的不同方式。

## 1. inject-via-java-api

使用 Java API（JNI）将 JuiceAgent 注入正在运行的 JVM，无需使用 `injector.exe`。

- [HowToUse.md](./inject-via-java-api/HowToUse.md)
- [如何使用.md](./inject-via-java-api/如何使用.md)

## 2. load-via-injector-exe

使用 `injector.exe` 将 JAR 注入正在运行的 JVM。

- [HowToUse.md](./load-via-injector-exe/HowToUse.md)
- [如何使用.md](./load-via-injector-exe/如何使用.md)

## 3. retransform-class

注入后在运行时热替换（retransform）已加载的类，无需重启 JVM。

- [HowToUse.md](./retransform-class/HowToUse.md)
- [如何使用.md](./retransform-class/如何使用.md)
