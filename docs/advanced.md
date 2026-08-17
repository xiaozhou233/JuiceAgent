# Advanced

## Load Custom API / Directly Load Jar
WARN: This feature didn't implement yet.

### Describe
JuiceAgent loads custom JARs through the following workflow:

1. Inject `libloader` into the target process.
2. Load **JuiceAgent-API** and execute its initialization entry point.
3. Invoke the `JarLoader` from **JuiceAgent-API** to load the specified JAR into the JVM.

The second step is fully customizable. You can replace **JuiceAgent-API** with your own JAR and call any class method you specify.

### Configuration

Set the following environment variables before launching `libloader`:

```bat
set JUICEAGENT_API_ENABLED=true
set JUICEAGENT_API_JAR=<path-to-your-jar>
set JUICEAGENT_API_CLASS=<target-class-name>
set JUICEAGENT_API_METHOD=<target-method-name>
```