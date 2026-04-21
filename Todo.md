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