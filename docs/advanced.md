# Advanced

## Load Custom API / Directly Load Jar

JuiceAgent runs in **BuiltIn** mode by default: `libloader` registers the embedded
JuiceAgent-API jar, the agent bootstraps it, and the JarLoader module loads your JAR
and invokes its entry method.

If you don't need JuiceAgent-API — for example you want to provide your own bootstrap
and call an arbitrary class/method — set `JarLoader.Mode` to **`CustomJar`**:

```toml
[JuiceAgent]
Version = 2

[JarLoader]
Mode = "CustomJar"
JarPath = "./my-agent.jar"
EntryClass = "com.example.MyBootstrap"
EntryMethod = "start"
```

Workflow in `CustomJar` mode:

1. `libloader` attaches to the target JVM and reads `config.toml`.
2. It adds `JarPath` to the system class loader search path. JuiceAgent-API is **not** loaded.
3. It invokes `EntryClass.EntryMethod`.

`EntryMethod` must be a `public static void` method that takes no arguments.

> `JarLoader.Enabled` and `JarLoader.InjectionDir` only apply to `BuiltIn` mode; they are
> ignored in `CustomJar` mode.

See [Configuration](config.md) for the complete list of keys.
