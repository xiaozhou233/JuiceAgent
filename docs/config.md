# Configuration (`config.toml`)

JuiceAgent reads a single `config.toml` from the **runtime directory** (the directory
passed to the injector / loader; the current working directory is used when none is
given). Paths that start with `./` are resolved against the runtime directory;
absolute paths are used as-is.

The current schema version is **2**. `JuiceAgent.Version` must match the version
expected by the binaries, otherwise the loader stops with a version mismatch error.

## Example

```toml
[JuiceAgent]
Version = 2
JuiceAgentNativeLibraryPath = ""

[JarLoader]
Enabled = true
Mode = "BuiltIn"
InjectionDir = "./injection"
JarPath = "./MyCustomJar.jar"
EntryClass = "Example.Main"
EntryMethod = "run"
```

## `[JuiceAgent]`

| Key | Type | Required | Default | Description |
| --- | --- | --- | --- | --- |
| `Version` | integer | yes | — | Configuration schema version. Must be `2`. |
| `JuiceAgentNativeLibraryPath` | string (path) | no | `./libagent.dll` | Path to `libagent.dll`, loaded by the bootstrap after injection. |

## `[JarLoader]`

| Key | Type | Required | Default | Description |
| --- | --- | --- | --- | --- |
| `Mode` | string | yes | — | How the target JAR is registered: `BuiltIn` or `CustomJar`. |
| `Enabled` | boolean | no | `false` | Run the JarLoader module after the agent is initialized (`BuiltIn` mode only). When `false`, no JAR is loaded and no entry method is invoked. |
| `InjectionDir` | string (path) | no | `./injection` | Directory of dependency JARs loaded before the main JAR (`BuiltIn` mode). |
| `JarPath` | string (path) | see below | `./Entry.jar` (`BuiltIn`) | The JAR to load. |
| `EntryClass` | string | see below | `Example.Main` (`BuiltIn`) | Entry class to invoke after loading the JAR. |
| `EntryMethod` | string | see below | `start` (`BuiltIn`) | Entry method, must be a `public static void` method with no parameters. |

> In `CustomJar` mode, `JarPath`, `EntryClass` and `EntryMethod` are required (there are
> no defaults); `Enabled` and `InjectionDir` are not used.

## Modes

### `BuiltIn` (recommended)

1. `libloader` extracts the embedded **JuiceAgent-API** jar and adds it to the system class loader.
2. The agent bootstraps JuiceAgent-API (`JuiceAgentBootstrap.start`).
3. If `Enabled = true`, the JarLoader module loads every `*.jar` from `InjectionDir`, then loads `JarPath`, and invokes `EntryClass.EntryMethod`.

This is the mode used by most users: keep JuiceAgent-API and just supply the JAR and
entry point to run.

### `CustomJar`

1. `libloader` adds `JarPath` itself to the system class loader. **JuiceAgent-API is not used.**
2. It invokes `EntryClass.EntryMethod` directly.

Use this when you want to bring your own bootstrap and call an arbitrary class/method.
See [Advanced](advanced.md#load-custom-api--directly-load-jar) for details.
