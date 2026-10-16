# Quick Start for Agents

This guide tells an **agent** how to set up a JuiceAgent injection for a user:
ask the right questions, generate the files, run the injection, and verify it.

Work through the steps in order. The result is a self-contained runtime
directory called `app`, plus a short report to the user.

## Ground rules

- Only inject into a JVM that the user owns or is authorized to test. If that is
  unclear, stop and ask before doing anything else.
- Do not modify the JuiceAgent native sources. Configuration and payload changes
  cover almost every request.
- Keep every file in one runtime directory: `config.toml`, both JARs and the
  four binaries. Relative paths only behave predictably when they sit next to
  each other (see [Step 4](#step-4--write-configtoml)).
- Never guess a PID. Confirm it with the user or read it from the process list.

## Project overview

JuiceAgent is a JVMTI-based runtime injection framework for Windows. It injects
into a running JVM even when `DisableAttachMechanism=true` and without a
`-javaagent` flag, loads a JAR and calls an entry point, and exposes native APIs
to redefine/retransform classes, query loaded classes and read bytecode.

## Step 1 — Check the environment (do this yourself, do not ask the user)

**1.1 JDK.** `javac`, `jar` and `jps` ship with the JDK, so a full JDK is
required — a bare JRE is not enough.

```
java -version
```

If the JDK is missing or older than 1.8, ask the user to install JDK 1.8 or
higher.

**1.2 JuiceAgent binaries.** When built from source they are in `build/bin`
(release archives put them wherever they were extracted):

```
ls build/bin
```

Expected files:

```
injector.exe   libinject.dll   libloader.dll   libagent.dll
```

If they are missing, ask the user to pick one of:

- **Method 1 (recommended):** download the latest release from
  https://github.com/xiaozhou233/JuiceAgent/releases/latest
- **Method 2:** build from source — see the "How To Build" section of `README.md`

**1.3 Create the runtime directory.** Copy the four files into one directory and
call it `app` for the rest of this guide. A complete `app` looks like:

```
app/
├─ injector.exe   libinject.dll   libloader.dll   libagent.dll
├─ config.toml                  # Step 4
├─ payload.jar                  # Step 3 (Q3)
├─ injection/                   # Step 3 (Q5), dependency JARs; BuiltIn mode only
├─ src/                         # payload source, if you generate it
└─ build/                       # compiler output, one subdirectory per artifact
   ├─ target/                   # demo target classes
   └─ payload/                  # payload classes
```

## Step 2 — Gather requirements (ask the user)

Ask these six questions and translate them into the user's language. Each
answer maps to a concrete decision, so do not skip any of them.

| # | Question | Decides |
| --- | --- | --- |
| Q1 | Which process do you want to inject into? (PID / process name / start a demo target — recommended) | the target and its working directory |
| Q2 | Is the target a standard, unmodified JVM? | whether injection will work at all |
| Q3 | What should the injected code do? (verify the link / your own code / modify bytecode) | the payload JAR, entry class and method |
| Q4 | Does the payload need JuiceAgent-API? (yes / no / don't know) | `JarLoader.Mode` |
| Q5 | Does the payload depend on other libraries? (yes / no) | `JarLoader.InjectionDir` |
| Q6 | How should the injection be triggered? (`injector.exe` / JNI JavaInjector) | the commands in Step 5 |

## Step 3 — Apply the answers

### Q1 — Target process

- **PID:** remember it for Step 5.
- **Process name:** resolve it to a PID with `jps -l`. Every JVM appears as
  `java.exe`, so match the main class or JAR path in the `jps -l` output, or use
  `Get-Process <name>` on Windows.
- **Start a demo target (recommended when the user has no target):** write
  `app/target.java`:

  ```java
  import java.lang.management.ManagementFactory;

  public class target {
      public static void main(String[] args) {
          String pid = ManagementFactory.getRuntimeMXBean().getName().split("@")[0];
          try {
              while (true) {
                  System.out.println("[Target] I am the target JVM (PID: " + pid + ")");
                  Thread.sleep(1000);
              }
          } catch (Exception e) {
              e.printStackTrace();
          }
      }
  }
  ```

  Compile and run it, **with `app` as the working directory**:

  ```
  javac -encoding UTF-8 --release 8 -d app/build/target app/target.java
  ```

  ```powershell
  Start-Process -FilePath java -ArgumentList "-cp","build/target","target" -WorkingDirectory "<path-to-app>"
  ```

  The target prints its PID every second; `jps -l` also lists it.

> Starting the target with `app` as its working directory is not cosmetic. See
> [Step 4](#step-4--write-configtoml) for why the working directory decides which
> `config.toml` the injected loader finds.

### Q2 — Standard JVM

- **Standard JVM / `DisableAttachMechanism=true`:** continue — JuiceAgent is
  designed for exactly this case.
- **Custom or modified JVM:** continue, but warn the user that JuiceAgent is
  only tested against a regular JVM and may not work.

### Q3 — Payload

**Verify the link.** Write `app/src/Main.java`:

```java
public class Main {
    public static void run() {
        System.out.println("[Demo] Injection succeeded! Hello from JuiceAgent.");
        System.out.println("[Demo] Current thread: " + Thread.currentThread().getName());
    }
}
```

Build it into `app/payload.jar`. Each artifact gets its own output directory, so
packaging the payload never picks up the demo target's classes:

```
javac -encoding UTF-8 --release 8 -d app/build/payload app/src/Main.java
jar --create --file app/payload.jar -C app/build/payload .
```

The class name `Main` and method name `run` are needed in Step 4.

**Own code.** Ask the user for the payload JAR and record its fully qualified
entry class and method. The entry method must be `public`, `static` and take no
arguments (the project documents this as `public static void`; the return value
is ignored). It is resolved by reflection, so a non-public, non-static or
parameterized method is only reported as a failure at invocation time — the JAR
itself still loads.

**Modify bytecode.** Point the user at `examples/retransform-class`. Payloads
that redefine or retransform classes use JuiceAgent-API, so Q4 must resolve to
`BuiltIn`.

### Q4 — JuiceAgent-API

| Answer | `JarLoader.Mode` |
| --- | --- |
| Yes | `BuiltIn` |
| No | `CustomJar` |
| I don't know | `BuiltIn` |

`BuiltIn` (recommended) runs in two stages, and **both stages read
`config.toml` from the runtime directory**:

1. `libloader` extracts the embedded JuiceAgent-API JAR into `%TEMP%`, adds it
   to the system class loader and calls `JuiceAgentBootstrap.start`, which loads
   `libagent.dll` (from `JuiceAgentNativeLibraryPath`) and initializes the JVMTI
   agent.
2. The agent's `JarLoaderService` reads the `JarLoader.*` keys again, then the
   `JarLoader` module adds every `*.jar` in `InjectionDir` to the system class
   loader, loads `JarPath` and invokes `EntryClass.EntryMethod`.

`Enabled` and `InjectionDir` are used in `BuiltIn` mode only. Omitting a key
silently falls back to a default, which is a common source of confusion:
`InjectionDir` defaults to `./injection`, `JarPath` to `./Entry.jar`,
`EntryClass` to `Example.Main` and `EntryMethod` to `start`.

`CustomJar` skips JuiceAgent-API and `libagent.dll` entirely: `libloader` adds
`JarPath` itself to the system class loader and invokes the entry method. In this
mode `Enabled` and `InjectionDir` are ignored, so any library the payload needs
must be bundled inside the payload JAR (or already on the target's classpath).

### Q5 — Other libraries

- **Yes:** create `app/injection/` and copy the dependency JARs into it. Every
  `*.jar` there is loaded before the payload (`BuiltIn` mode only).
- **No:** still create the empty directory if the config references it, or drop
  `InjectionDir` from the config.

### Q6 — Injection way

- **`injector.exe`:** nothing to prepare, continue to Step 4.
- **JNI JavaInjector:** copy the whole `examples/inject-via-java-api/injector`
  directory to `app/injector/`, then build it:

  ```
  app\injector\build.bat
  ```

  That directory ships a prebuilt `JuiceAgent-JavaInjector-1.0-SNAPSHOT.jar`;
  `build.bat` merges the example's `JavaInjector.class` into it and sets the main
  class, so keep the JAR in place.

## Step 4 — Write config.toml

The file must be named exactly `config.toml` and placed in the directory that
`libloader` treats as the **runtime directory** (see the table below). Do not
rename it and do not scatter it across directories.

```toml
# See docs/config.md for more information:
# https://github.com/xiaozhou233/JuiceAgent/blob/main/docs/config.md

[JuiceAgent]
Version = 2
JuiceAgentNativeLibraryPath = ""

[JarLoader]
# BuiltIn: use the built-in jar loader (recommended; supports JuiceAgent-API)
# CustomJar: load the JAR directly, without JuiceAgent-API
# https://github.com/xiaozhou233/JuiceAgent/blob/main/docs/advanced.md#load-custom-api--directly-load-jar
Enabled = true

# Q4
Mode = "BuiltIn"

# BuiltIn mode only; Q5
InjectionDir = "./injection"

# Q3
JarPath = "./payload.jar"
EntryClass = "Main"
EntryMethod = "run"
```

- `Version` must stay `2`. A mismatch makes the loader refuse to start.
- `JuiceAgentNativeLibraryPath = ""` means "`libagent.dll` in the runtime
  directory". Change it to an absolute path only if `libagent.dll` lives
  elsewhere.
- `Mode = "CustomJar"` requires `JarPath`, `EntryClass` and `EntryMethod`
  (there are no defaults in that mode); `Enabled` and `InjectionDir` are unused.
- Paths starting with `./` are resolved against the runtime directory; absolute
  paths are used as-is.

### Which directory is the runtime directory?

This is where most failed setups go wrong: `libloader` does not take the config
directory from the `injector.exe` command line.

| How you inject | Directory `config.toml` is read from |
| --- | --- |
| Interactive `injector.exe` (type the PID) | the **injector process's** current directory — so `cd app` first |
| `injector.exe <pid> <libloader.dll> [libinject.dll]` | the **target JVM's** current directory (no directory is passed) |
| JavaInjector (JNI) | the `config_path` you enter; an empty value falls back to the target JVM's working directory |

Consequences:

- With the argument form, `config.toml` **and** every `./` path inside it
  (`JarPath`, `InjectionDir`, `libagent.dll`) resolve against the target JVM's
  working directory.
- If the user cannot control the target's working directory, use absolute paths
  in `config.toml` — including `JuiceAgentNativeLibraryPath`.
- JNI injection is the most predictable option, because the config directory is
  passed explicitly.

## Step 5 — Run the injection

### 5.1 Use `injector.exe`

```
cd app
```

`cd app` matters: `injector.exe` loads `libinject.dll` **by name from the
current directory**, and in interactive mode the current directory is also the
runtime directory.

**Interactive:**

```
.\injector.exe
```

It prints the `jps -l` output, then `Input PID:`. Type the PID of the target JVM
and press Enter.

> The program always ends with `Press any key to exit...` and waits for stdin.
> If you drive it from a script, feed it a newline after the PID and do not wait
> for the process to exit on its own.

**With arguments:**

```
.\injector.exe <PID> "<path-to-app>\libloader.dll" "<path-to-app>\libinject.dll"
```

The third argument is the path to **`libinject.dll`**, not a config directory.
Omit it to let `injector.exe` load `libinject.dll` from the current directory.
Remember that this form reads `config.toml` from the *target's* working
directory (Step 4).

### 5.2 Use JNI JavaInjector

Run it from `app`, because the example loads `./libinject.dll`:

```
java -cp injector\JuiceAgent-JavaInjector-1.0-SNAPSHOT.jar JavaInjector
```

```
PID: 1234
Dll: <path-to-app>\libloader.dll
config_path: <path-to-app>
```

Enter the target PID, the path to `libloader.dll`, and the directory containing
`config.toml`. Leaving `config_path` empty behaves like the argument form above:
the runtime directory falls back to the **target JVM's** working directory, so
always pass the directory explicitly.

## Step 6 — Verify

Verification has two independent sources. Do not confuse them: the
`[JuiceAgent JarLoader]` progress lines are printed to the **target's stdout**,
while the diagnostic logs are written to **files**.

**1. Target stdout** — this is the user-visible proof:

```
[JuiceAgent API] RuntimeDir : <the directory actually used>
[JuiceAgent JarLoader] Loading entry jar: ...
[JuiceAgent JarLoader] Resolving class: Main
[JuiceAgent JarLoader] Invoking method: Main.run
[JuiceAgent JarLoader] Entry method invoked successfully.
[Demo] Injection succeeded! Hello from JuiceAgent.
```

The `[JuiceAgent API]` banner prints the effective runtime directory and the
native library path, which is the fastest way to confirm the Step 4 rules.

**2. Log files** — two files are written **relative to the target JVM's working
directory** and truncated on every injection:

| File | Look for |
| --- | --- |
| `libloader.log` | `DllMain: DLL_PROCESS_ATTACH, runtime_dir:` and `[PreLoad] Runtime directory:` |
| `libagent.log` | `[JuiceAgent] Added JVMTI capabilities`, `[JarLoaderService] Loaded jar:`, and `JNI Exception occurred at:` when the payload itself throws |

If the target has no console (a service or a detached window), the stdout
markers are invisible — verify through the log files instead.

Report the outcome to the user, including the exact `app` path, the injection
command used, and the observed output.

## Troubleshooting

| Symptom | Likely cause |
| --- | --- |
| Injection reports success but nothing happens | `config.toml` is not in the directory `libloader` actually used — see Step 4 |
| `RuntimeDir :` in the banner is not `app` | The runtime directory fell back to the target JVM's working directory — see Step 4 |
| `[JuiceAgent API] JuiceAgent library not found at: <path>` | `libagent.dll` could not be resolved — set an absolute `JuiceAgentNativeLibraryPath`, or fix the runtime directory |
| `Failed to load injector library` / error 2 | `injector.exe` was not started from `app`, and no `libinject.dll` path was passed |
| `Failed to open DLL file` | The `libloader.dll` path is wrong |
| `libloader.log` / `libagent.log` seem missing | They are written in the target JVM's working directory, not your shell's |
| Loader refuses to start | `[JuiceAgent] Version` is not `2` |
| `Missing required config key: JarLoader.Mode` | `JarLoader.Mode` was omitted |
| Errors about missing `JarPath` / `EntryClass` / `EntryMethod` | `Mode = "CustomJar"` requires all three |
| `Failed to load jar: ...\Entry.jar` | `JarPath` was omitted and the default was used |
| The JAR loads but the entry call fails | The method is not `public static` with no parameters, or `EntryClass` is not fully qualified |
| `JNI Exception occurred at: CallStaticVoidMethod failed` | The payload threw — read the stack trace printed next to it |
| The injected process hangs for ~30 s, then `Failed to attach to JVM!` | The target process is not a JVM (the loader retries 30 × 1 s before giving up) |
| `jps` does not list the target | The target is 32-bit (JuiceAgent ships x64 only) or runs with different privileges |
| Injection fails on an elevated target | Run `injector.exe` elevated too |
| Antivirus flags the binaries | Expected false positive — see `docs/Antivirus-False-Positive.md` |
| Native-access warning on JDK 22+ | The payload loads a native library; the target JVM needs `--enable-native-access=ALL-UNNAMED` |

## Done checklist

- [ ] A JDK 1.8+ with `javac`, `jar` and `jps` is available
- [ ] `app/` contains all four JuiceAgent binaries
- [ ] The payload JAR exists and its entry method is `public static` and takes no arguments
- [ ] `config.toml` is in the directory `libloader` actually uses (Step 4), `Version = 2`, and every key matches the Q3–Q5 answers
- [ ] The injection command reported success
- [ ] The `[JuiceAgent API]` banner shows the expected `RuntimeDir`, and the payload output appears with no errors in the logs
