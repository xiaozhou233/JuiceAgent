# Changelog / 更新日志

本文件记录 JuiceAgent 的版本变更。更早的版本请见 [GitHub Releases](https://github.com/xiaozhou233/JuiceAgent/releases)。

This file records notable changes to JuiceAgent. For older releases, see [GitHub Releases](https://github.com/xiaozhou233/JuiceAgent/releases).

## 中文

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
