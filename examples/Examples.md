# Examples

A collection of JuiceAgent examples demonstrating different ways to inject
JuiceAgent into a running JVM process.

## 1. inject-via-java-api

Inject JuiceAgent into a running JVM using the Java API (JNI), without needing `injector.exe`.

- [HowToUse.md](./inject-via-java-api/HowToUse.md)
- [如何使用.md](./inject-via-java-api/如何使用.md)

## 2. load-via-injector-exe

Inject a JAR into a running JVM using `injector.exe`.

- [HowToUse.md](./load-via-injector-exe/HowToUse.md)
- [如何使用.md](./load-via-injector-exe/如何使用.md)

## 3. retransform-class

Retransform an already-loaded class at runtime after injection, without restarting the JVM.

- [HowToUse.md](./retransform-class/HowToUse.md)
- [如何使用.md](./retransform-class/如何使用.md)
