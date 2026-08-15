# How to Use — inject-via-java-api (JuiceAgent)

This example demonstrates injecting JuiceAgent into a running JVM process
using the Java API (JNI), without needing `injector.exe`.

## Prerequisites

- Java 8+ installed and available in `PATH`
- JuiceAgent native DLLs (`libinject.dll`, `libloader.dll`, `libagent.dll`)
  placed in the same directory as this example
- A `config.toml` file configured for your use case

## Quick Start

### Step 1: Build

Run `build.bat` to compile both the injector and the target jars:

```
> build.bat
```

### Step 2: Run the Target JVM

Open a terminal and start the target JVM:

```
> run.bat
```

The target JVM will start and print its PID periodically. Keep it running.

### Step 3: Run the Injector

Open another terminal and run the injector:

```
> run_injector.bat
```

You will be prompted to enter:

- `PID` : the target JVM process ID (shown in the target terminal)
- `DLL` : path to `libinject.dll` (e.g. `.\libinject.dll`)
- `config_path` : directory containing `config.toml` (leave empty for current dir)

### Step 4: Verify

If the injection is successful, the target JVM will load the JAR specified
in `config.toml` and execute its entry method.

## Files

| File | Description |
| --- | --- |
| `build.bat` | Build all jars (injector + target) |
| `run.bat` | Start the target JVM |
| `run_injector.bat` | Run the JavaInjector to inject into the target JVM |
| `config.toml` | JuiceAgent configuration file |
| `injector/` | JavaInjector source code and build script |
| `target/` | Target JVM source code and build script |

## Configuration (config.toml)

```toml
[JuiceAgent]
Version = 1

[JuiceAgent.Loader]
JuiceAgentNativeLibraryPath = ""

[JuiceAgent.Modules]

[JuiceAgent.Modules.JarLoader]
Enabled = true
InjectionDir = "./injection"
JarPath = "./demo.jar"
EntryClass = "Example.Main"
EntryMethod = "run"
```

- `JarPath`: the JAR to load into the target JVM
- `EntryClass`: the fully qualified class name with the entry method
- `EntryMethod`: must be a `public static void` method with no parameters
- `InjectionDir`: directory containing dependency JARs loaded before the main JAR

## Troubleshooting

- `Error: JuiceAgent-JavaInjector-1.0-SNAPSHOT.jar not found`:
  Run `build.bat` first to compile the injector.

- `java.lang.UnsatisfiedLinkError`:
  Ensure `libinject.dll` is in the current directory or provide the full path.

- Injection has no effect:
  Verify that `config.toml` is in the directory you specified as `config_path`,
  and that the paths inside `config.toml` are correct relative to that directory.
