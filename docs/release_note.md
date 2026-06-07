# Release Notes

## v2.1.2 - 2026-05-20

- **版本级别**: patch
- **Git 提交消息**: docs: 为所有头文件接口添加中文 Doxygen 文档注释
- **Git Tag**: v2.1.2

### 变更摘要

- 为项目所有头文件添加完整的中文 Doxygen 文档注释
- 注释覆盖文件级（@file/@brief/@author/@version/@details）、类/结构体级（@brief/@details/@tparam/@note）、方法级（@brief/@param/@return）以及成员变量和枚举值的行尾 ///< 注释
- 更新 CMakeLists.txt 版本号至 v2.1.2

## v2.1.1 - 2026-05-18

- **版本级别**: patch
- **Git 提交消息**: chore: 统一 CMake 导出文件命名
- **Git Tag**: v2.1.1

### 变更摘要

- 将安装导出的 CMake targets 文件改为 `galayUtilsConfigTargets.cmake`，同步 package config 的 include 路径
- Release 安装现在生成 `galayUtilsConfigTargets-release.cmake`，与新的驼峰导出文件命名保持一致
- 将 CMake project 版本提升到 `2.1.1`，对齐本次发布 tag

## v2.1.0 - 2026-04-29

- 版本级别：中版本（minor）
- Git 提交消息：`refactor: 移除异步限流器并发布 v2.1.0`
- Git Tag：`v2.1.0`
- 自述摘要：
  - 新增 `IniParser`、`TomlParser` 与 `parser_manager.hpp` 统一出口，`ParserManager` 默认支持 `.conf`、`.ini`、`.env`、`.toml`。
  - 拆分 parser 模块头文件，保留 `parser.hpp` 兼容入口，并同步更新 umbrella header、C++ 模块入口与使用文档。
  - 移除 `RateLimiter` 的异步 `acquire()` / awaitable 路径，只保留同步非阻塞 `tryAcquire()` 接口。
  - 移除限流器对 `galay-kernel` 与 `concurrentqueue/moodycamel` 的依赖，测试构建不再查找或链接 `galay-kernel`。

## v2.1.3 - 2026-05-24

- **版本级别**: patch
- **Git 提交消息**: fix: 修复 TOML 多行数组解析与头文件编译问题
- **Git Tag**: v2.1.3

### 变更摘要

- 修复 `TomlParser` 解析多行 TOML 数组失败的问题，支持数组尾逗号、数组内注释和空行
- 修复 TOML 数组字符串元素包含逗号时 `getArray()` 误拆分的问题，并加强字符串引号校验与 literal string 反斜杠处理
- 修复 `defn.hpp` 与 `backtrace/trace.hpp` 中重复声明导致 `test_all` 无法编译的问题
- 修复 `hmac.hpp` 单独包含时缺少 `<vector>` 依赖的问题
- 同步更新 `TomlParser` 文档，并将 CMake project 版本提升到 `2.1.3`

## v3.0.0 - 2026-06-07

- **版本级别**: 大版本（major）
- **Git 提交消息**: refactor: 发布 v3.0.0 并收束工具模块目录
- **Git Tag**: v3.0.0

### 变更摘要

- 将公开头文件收敛到 canonical include 目录，包括 `core`、`cache`、`tool`、`process`、`algorithm`、`config`、`crypto`、`encoding` 等，不保留旧路径兼容头
- 新增 `LruCache`、`ByteQueueView`、`RingBuffer`、split-block `BloomFilter<T>`、`RandomGenerator` 以及轻量时间工具，补充测试、benchmark 和文档
- 将 `ThreadPool` 任务队列切换到 moodycamel `BlockingConcurrentQueue`，CMake 包配置暴露 `concurrentqueue` 头依赖
- 将单体 `test_all.cpp` 拆分为按领域组织的 CTest target，并扩展模块导入烟测
- 修复系统测试固定 `/tmp` 文件污染、字符串/随机工具边界处理等问题
- 将 CMake project 版本提升到 `3.0.0`，对齐本次主版本 tag
