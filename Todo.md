# TODO - 开发待办事项
这个文档用来记录开发时的待办事项以及以后的开发计划。
---

## 2026-04-16
 - [ ] ~~把读取toml的库换成支持toml 1.0的库~~
 - [x] 在  `includes/JuiceAgent` 文件夹下实现 `Config.hpp` 配置系统
 - [ ] ~~新建测试 读取配置文件 (放弃)~~

 - [ ] 重构 `libloader` 的配置系统

## 2026-04-17
 - [x] 重构项目为模块化项目
    - [x] `libloader` 模块只负责加载`JuiceAgent-API`和`JuiceAgent`, Jar文件由`JuiceAgent`接管
    - [x] `libloader` 读取配置文件并传参`JuiceAgentNativePath`和`ConfigDir`到 `JuiceAgent` 中，由`JuiceAgent`读取配置文件继续进行操作

## 2026-04-19
 - [x] 在 `JuiceAgent/Utils.hpp`做序列化操作
 - [x] 统一调用 module的Java实现

## 2026-04-20
 - [x] 修改文档

## 2026-08-14
### 待修复问题（已合并 ai 分支 bb5b215 修复大部分）
 - [x] **功能Bug**：`retransformClass` 把新字节码写入 `classFileDataMap`，但 `patch_bytecodes` 读取的是 `pendingRetransform`（且无任何代码写入它）→ 重变换补丁从未生效
 - [x] **并发问题**：`pendingRetransformMutex` 声明后从未使用，`pendingRetransform` 读写无锁保护，存在数据竞争
 - [x] **悬垂指针**：`capture_bytecodes` 将 JVMTI 回调的本地引用（clazz/classloader/protection_domain）直接存入全局 map，回调返回后即失效
 - [x] **引用泄漏**：`retransformClass` 中 `NewGlobalRef` 从不释放，每次调用泄漏一个全局引用
 - [x] **本地引用泄漏**：`Loader.cpp` 的 `invoke_juiceagent_init` 中 `jArgs` 未调用 `DeleteLocalRef`
 - [ ] **缓存无上限**：`classFileDataMap` 只增不减，无清理/刷新机制
 - [x] **崩溃风险**：`patch_bytecodes` 未检查 `jvmti Allocate` 返回值就 `memcpy`，分配失败会写入空指针
 - [x] **崩溃风险**：`defineClass` / `getLoadedClasses` JNI 缺少空指针与异常检查
 - [x] **性能**：`ClassFileLoadHook` 对每个类加载都无条件分发两次事件（readonly + mutable），mutable 无订阅者，纯浪费
 - [x] **性能**：三处重复实现「遍历全部已加载类找类」逻辑，每个 O(n) 且构造临时 `std::string`
 - [x] **健壮性**：`InitLogger` 被多处调用，plog 会重复初始化
 - [ ] **无效调用**：`agent_onload.cpp` 中 `AddCapabilities(&caps)` 传全 0 的空 caps
 - [x] **构建**：CMake `GLOB_RECURSE` 缺少 `CONFIGURE_DEPENDS`，新增源文件不会触发重新配置

### 后续待优化项（未做）
 - [ ] 类加载热点路径：`capture_bytecodes` / `patch_bytecodes` 每次类加载都加锁查 map，可加 `std::atomic<bool>` 快速通道跳过
 - [ ] `agent_onload.cpp` 中 `AddCapabilities(&caps)`（空 caps）应删除
 - [ ] 死代码：空文件 `instrumentation.cpp`；`MethodEntry`/`MethodExit` 回调从未注册使能；`services.hpp` 中 `static` 应改 `inline`
 - [ ] `classFileDataMap` 无清理 API，长期运行会无限增长
 - [ ] `JvmManager::attach` 30 次 × 1s 重试，最坏阻塞 30 秒
 - [ ] `libinject.c` 固定写满 `MAX_PATH`(260) 字节而非 `strlen+1`
 - [ ] `Agent::init(std::string& runtime_dir)` 应改 `const std::string&`
 - [ ] Release 构建可加 strip 缩小二进制体积