# Changelog / 更新日志

本文件记录 JuiceAgent 的版本变更。更早的版本请见 [GitHub Releases](https://github.com/xiaozhou233/JuiceAgent/releases)。

This file records notable changes to JuiceAgent. For older releases, see [GitHub Releases](https://github.com/xiaozhou233/JuiceAgent/releases).

## 中文

### [5.0.1] - 2026-10-16

#### 🐛 修复

- 修复配置错误导致目标 JVM 崩溃：`libloader.dll` 是反射加载的，其展开表未被登记，C++ 异常在其中既无法 unwind 也无法捕获。现改为配置读取非抛出 + 解析走无异常模式，并以 `-fno-exceptions` 编译 libloader，任何会抛异常的代码在编译期即失败。

#### ✨ 新功能

- 事件总线新增 `subscribeFront` / `listenFront`（监听者顺序能力）。

#### 🔧 内部改动

- 用约 60 行类型安全事件总线替换 `eventpp`（移除 23 个 in-tree 头文件，约 144 KB）。
- 移除无引用的 `MethodEntry` / `MethodExit` 事件定义。

#### 📝 文档

- 新增 `docs/config.md` 与 `docs/update/`；重写 `docs/advanced.md`；README 配置示例升级到 v2。

#### 🔁 CI

- 新增 GitHub Actions 工作流（构建 + 集成测试 + 打包 + 上传 artifact）与 `tests/run-tests.ps1`。
- 构建改为静态链接 C/C++ 运行时。

### [5.0.0] - 2026-10-15

#### ⚠️ 破坏性变更

- 配置 schema 由 v1 升级到 v2（`CONFIG_VERSION = 2`）：旧配置会被版本校验拒绝，需按 [v5.0.0 发布说明](v5.0.0.md) 迁移。

#### ✨ 新功能

- 新增 `JarLoader.Mode`：`BuiltIn`（内置 JuiceAgent-API）与 `CustomJar`（直接加载自定义 JAR）两种加载模式。
- 新增 `JarLoader.Enabled` 开关，可在 `BuiltIn` 模式下关闭 JarLoader 模块。
- 新增配置解析与访问的错误日志。

#### 🐛 修复

- 自定义 JAR 入口类名规范化（`.` → `/`），修复 `NoClassDefFoundError`。
- `tests/copy-bin.bat` 在被占用时会静默失败的问题。
- `GetClassBytesByName` 集成测试未先加载目标类的问题。

#### 🔧 内部改动

- TOML 解析器由 `toml11` 迁移到 `toml++`。
- 移除未使用的 `LoaderConfig` 相关辅助函数。

#### 📝 文档

- 新增 `docs/config.md`（v2 配置参考）。
- 重写 `docs/advanced.md`；README 与示例文档升级到 v2。

### [4.1.2] - 2026-10-11

#### ✨ 新功能

- 新增基于版本的打包脚本（`script/package.bat` + `script/package.cmake`）。
- `Version.txt` 改为 `Major` / `Minor` / `Patch` 结构化格式。

#### 🐛 修复

- 修复 `getClassBytesByName` 在部分环境下无法获取类的问题（改用 JVMTI 的 `findLoadedClass`）。

#### 📝 文档

- 新增 `docs/Inject.md`；修正文档路径。

## English

### [5.0.1] - 2026-10-16

#### 🐛 Fixes

- Fix a config error crashing the target JVM: `libloader.dll` is reflectively loaded, so its unwind tables are never registered and a C++ exception cannot be unwound or caught there. Config reads are now non-throwing, parsing runs in non-throwing mode, and libloader is compiled with `-fno-exceptions` so exception-capable code fails the build.

#### ✨ Features

- New `subscribeFront` / `listenFront` listener-ordering support on the event bus.

#### 🔧 Internal

- Replace `eventpp` with a ~60-line type-safe event bus (removes 23 in-tree headers, ~144 KB).
- Remove the unreferenced `MethodEntry` / `MethodExit` event definitions.

#### 📝 Docs

- Add `docs/config.md` and `docs/update/`; rewrite `docs/advanced.md`; update the README config examples to v2.

#### 🔁 CI

- Add a GitHub Actions workflow (build + integration tests + package + artifact upload) and `tests/run-tests.ps1`.
- Link the C/C++ runtime statically.

### [5.0.0] - 2026-10-15

#### ⚠️ Breaking Changes

- Configuration schema upgraded from v1 to v2 (`CONFIG_VERSION = 2`): older configs are rejected by the version check and must be migrated (see the [v5.0.0 release notes](v5.0.0.md)).

#### ✨ Features

- New `JarLoader.Mode`: `BuiltIn` (embedded JuiceAgent-API) and `CustomJar` (load your own JAR directly).
- New `JarLoader.Enabled` switch to turn the JarLoader module off in `BuiltIn` mode.
- Added logging for config parse and access errors.

#### 🐛 Fixes

- Normalize custom-jar entry class names (`.` → `/`), fixing `NoClassDefFoundError`.
- `tests/copy-bin.bat` no longer fails silently when a target file is locked.
- `GetClassBytesByName` integration test now loads the target class first.

#### 🔧 Internal

- Migrate the TOML parser from `toml11` to `toml++`.
- Remove the unused `LoaderConfig` helpers.

#### 📝 Docs

- Add `docs/config.md` (v2 configuration reference).
- Rewrite `docs/advanced.md`; update the READMEs and example guides to v2.

### [4.1.2] - 2026-10-11

#### ✨ Features

- Add a version-based packaging script (`script/package.bat` + `script/package.cmake`).
- `Version.txt` now uses a structured `Major` / `Minor` / `Patch` format.

#### 🐛 Fixes

- Fix `getClassBytesByName` failing in some environments (use JVMTI `findLoadedClass`).

#### 📝 Docs

- Add `docs/Inject.md`; fix documentation paths.
