# AGENTS.md

Guidance for AI coding agents (and humans) working in this repository.

## Project at a glance

JuiceAgent is a JVMTI-based runtime injection framework for Windows. It injects
into a running JVM (even with `DisableAttachMechanism=true`, no `-javaagent`),
loads a JAR and calls an entry point, and exposes native APIs to
redefine/retransform classes, query loaded classes and read bytecode.

Four binaries:

| Target | Output | Role |
| --- | --- | --- |
| `injector` | `injector.exe` | CLI driver: loads `libinject.dll` and injects `libloader.dll` into a target PID |
| `libinject` | `libinject.dll` | Reflective DLL injector (exports `inject`, plus JNI bindings) |
| `libloader` | `libloader.dll` | **Reflectively loaded** into the target JVM. Attaches to the JVM, reads `config.toml`, registers the embedded JuiceAgent-API jar, bootstraps the agent |
| `libagent` | `libagent.dll` | The JVMTI agent. Registers `ClassFileLoadHook` and implements the native side of the JuiceAgent-API |

## Toolchain

- Windows x64, MinGW-w64 GCC (the maintainer uses [w64devkit](https://github.com/skeeto/w64devkit)), Ninja, CMake 3.15+
- C++20
- JDK 17+ for the Java side. `--enable-native-access=ALL-UNNAMED` is required from
  JDK 22 on to keep `System.load` warning-free (a future JDK will deny native
  access without it).

## Build

```powershell
cmake --preset release      # or: --preset debug
cmake --build build
```

Binaries land in `build/bin`.

Alternatively, without presets:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Test

Integration tests live in `tests/`. They are plain `main()` programs (not JUnit)
that `System.load("resources/libagent.dll")` and call `JuiceAgent.init("")`.
Two of them need the `injection` module compiled into `out/production/injection`.

Build first, then:

```powershell
pwsh tests/run-tests.ps1
# or point it at a specific JDK:
pwsh tests/run-tests.ps1 -JdkHome "C:\Program Files\Zulu\zulu-25"
```

The runner compiles the modules, refreshes `tests/resources` from `build/bin`
(via `tests/copy-bin.bat`), compiles the tests, then runs every test and
**asserts on its output**.

> Running the tests rebuilds the tracked file `tests/injection.jar`. After a local
> run, restore it with `git checkout -- tests/injection.jar` before committing.

## Release

1. Bump `Version.txt` (`Major` / `Minor` / `Patch`).
2. Add `docs/update/vX.Y.Z.md` (bilingual release notes) and an entry in
   `docs/update/CHANGELOG.md`.
3. Commit the docs, then commit `Version X.Y.Z`, then tag `vX.Y.Z`.
4. Build the package and attach it to a GitHub Release:

   ```powershell
   cmake -P script/package.cmake      # -> dist/JuiceAgent_vX.Y.Z_x64.zip
   ```

   (`script/package.bat` wraps this but ends with `pause`.)

   On Linux, `script/package.sh` packs `injector`, `libloader.so` and
   `libagent.so` into `dist/JuiceAgent_vX.Y.Z_x64_linux.tar.gz`:

   ```bash
   ./script/package.sh
   ```

`tests/`, `build/`, `dist/` and `*.log` are gitignored; `dist/*.zip` and
`dist/*.tar.gz` are **not** committed.

## Hard rules (each of these was learned by breaking the build or a target JVM)

### 1. `libloader` must never throw a C++ exception

`libloader.dll` is loaded by a **reflective loader (manual mapping)**, so its
unwind tables are never registered with the OS. A C++ exception thrown inside it
**cannot be unwound or caught** - it crashes the target JVM with
`EXCEPTION_ILLEGAL_INSTRUCTION` (a `try/catch` there is useless, not a fix).

- The `loader` target is compiled with `-fno-exceptions` (and
  `SPDLOG_NO_EXCEPTIONS`) on purpose: any exception-capable code fails the build.
- Report problems with `spdlog` + an early `return`, never with `throw`.
- This also means library code that can throw (`toml::parse_file`,
  `std::filesystem`, allocations) must be used in a non-throwing way. toml++
  switches to the non-throwing `toml::parse_result` API automatically when
  exceptions are disabled.

### 2. Config reads are non-throwing

`includes/JuiceAgent/Config.hpp` intentionally has **no throwing getters**. Use:

```cpp
if (!cfg.contains("JarLoader.Mode")) { spdlog::error("Missing key: JarLoader.Mode"); return; }
const std::string mode = cfg.get<std::string>("JarLoader.Mode", "");
```

For required values, check `contains()` first and then `get(path, default)`.

### 3. Config schema changes are a coordinated edit

The current schema is v2 (`CONFIG_VERSION` in `includes/JuiceAgent/JuiceAgent.hpp`).
It is validated at load time, so a mismatch makes the loader refuse to start.
Changing the schema requires updating, together:

- `CONFIG_VERSION`
- `docs/config.md`
- the three `examples/*/config.toml`
- the config block in `README.md` **and** `README_ZH.md`

### 4. Keep the MinGW runtime out of the shipped DLLs

Do not introduce a dependency on `libstdc++-6.dll` / `libgcc_s_seh-1.dll` /
`libwinpthread-1.dll`. A DLL that needs them can pick up an incompatible copy
from elsewhere on the machine and fail to load (`UnsatisfiedLinkError: The
specified procedure could not be found`). w64devkit links the runtime statically;
CI passes `-static-libgcc -static-libstdc++` to match.

### 5. Tests assert on output, not exit codes

The test programs print diagnostics and **still exit 0** when a step fails. Match
an expected marker (e.g. `Bytes: 399`, `Modified!`, `Injection Invoked!`) -
checking `$LASTEXITCODE` alone will silently pass broken builds.

Notes when adding tests:

- `GetClassBytesByName` only resolves **already loaded** classes; load the target
  first (`Class.forName("Target")`).
- Tests that print to stderr make Windows PowerShell 5.1 unhappy; the runner sets
  `$ErrorActionPreference = "Continue"` and relies on explicit checks.

## Layout

```
src/injector        injector.exe
src/libinject       libinject.dll (reflective injector)
src/libloader       libloader.dll (runs in the target; exception-free)
src/libagent        libagent.dll (JVMTI agent)
  services/         IService implementations, self-registered via JUICEAGENT_REGISTER_SERVICE
  includes/eventbus.hpp   typed EventBus
includes/JuiceAgent Config.hpp / Logger.hpp / Utils.hpp / JuiceAgent.hpp (CONFIG_VERSION)
includes/toml++     vendored
includes/spdlog     vendored
includes/ptrace-inject-cpp  vendored Linux injector library (libptrace_inject.a)
tests/              integration tests + run-tests.ps1 + copy-bin.bat
docs/               config.md, advanced.md, Inject.md, update/ (release notes + changelog)
examples/           runnable end-to-end examples (each has config.toml + HowToUse)
script/             package.cmake / package.bat
```

## Conventions

### Services and the event bus

A service derives from `IService` (`src/libagent/includes/services/base.hpp`) and
self-registers at file scope:

```cpp
JUICEAGENT_REGISTER_SERVICE(::JuiceAgent::Services::MyService)
```

Subscribe with the typed bus - there is no `EventId` and no `void*` casting:

```cpp
listen<MyService, EventLoaded>(&MyService::onLoaded);      // append
listenFront<MyService, EventLoaded>(&MyService::onLoaded); // run before others
```

Publish with `EventBus::getInstance().post(event)`. Call `unlistenAll()` in
`onShutdown()`.

### Commit messages

`Type: subject`, then `- component: detail` bullets. Types used here:
`Feat`, `BugFix`, `Refactor`, `Docs`, `CI`, and `Version X.Y.Z` for the release
commit.

### Docs

`README.md` and `README_ZH.md` are kept in sync; keep both updated. Prose docs
live in `docs/` (English). Release notes and the changelog live in `docs/update/`.

### Line endings

Tracked text files are **LF in the index, CRLF in the working tree** (no
`.gitattributes`). When creating files, write CRLF to avoid noisy diffs.

## CI

`.github/workflows/ci.yml` has two jobs:

- `build-and-test` on `windows-latest`: JDK 25 (Temurin) + MinGW-w64 (MSYS2) +
  Ninja, then `configure -> build -> tests -> package`, uploading
  `dist/JuiceAgent_*.zip` as an artifact (14-day retention).
- `build-linux` on `ubuntu-latest`: GCC + Ninja, then
  `configure -> build -> package` only — the integration tests are Windows-only —
  uploading `dist/JuiceAgent_*.tar.gz`.

CI-specific notes:

- It builds with `-static-libgcc -static-libstdc++` and also copies any remaining
  non-system imported DLLs next to the binaries (see rule 4).
- The workflow prints the DLL import list - useful when a load error appears.
- GitHub's unauthenticated REST API is limited to 60 requests/hour, so avoid
  polling workflow status in a loop; install `gh` or use a token instead.
