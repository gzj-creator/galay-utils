# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- 新增非线程安全的通用 `LruCache`，支持容量 LRU 淘汰、惰性 TTL 淘汰、自定义哈希/比较器、淘汰回调和可选 expire-after-access。
- 为 `LruCache` 增加可编译期开关控制的统计收集，默认关闭统计以减少容量缓存热路径开销。
- 增加 LRU 单测和 benchmark，对比 LeetCode 常见 LRU 实现、默认容量模式、统计开启模式和 TTL 模式。
- 新增轻量时间工具 `StopWatch`、`Deadline` 和 `Backoff`，保持无线程、无调度器、无阻塞语义。
- 新增 `ByteQueueView` 与 `RingBuffer` 通用缓冲工具，覆盖流式解析、环绕读写、span 视图和 POSIX `iovec` 视图。
- 增加 ByteQueueView、RingBuffer benchmark，并补充对应边界单测与压测文档。
- 新增本地无锁 `RandomGenerator`，用于协程热路径或单线程上下文，避免共享 `Randomizer` 的 mutex 竞争。
- 新增模块生产化路线图和目录收敛计划，并完成目录/测试布局收敛与 core 工具硬化两个阶段。

### Changed
- 将系统时间戳与时间格式化接口从 `System` 迁移到 `Time`，`System` 不再保留时间相关 API。
- 将公开头文件进一步收敛到 `process`、`tool`、`algorithm` 等 canonical include 目录，不保留兼容头。
- 将 `ThreadPool`、`TaskWaiter`、`ObjectPool`、`BlockingObjectPool` 迁入 `galay-utils/tool/`，移除旧 `galay-utils/concurrency/` 公开入口且不保留兼容头。
- 将 `ThreadPool` 任务队列改为 moodycamel `BlockingConcurrentQueue`，提交路径不再使用 `std::mutex` / `std::condition_variable`，通过 CMake 暴露 `concurrentqueue` 头依赖，并移除无实际使用场景的 `ThreadSafeList` / `ListNode`。
- 将原单体 `test_all.cpp` 拆分为 `test/<area>/*_test.cpp` 分组 CTest target，并扩展模块导入烟测覆盖。
- 明确 `StringUtils`、`Time`、`TypeName`、`RandomGenerator`、`Randomizer` 的线程安全、阻塞与失败语义文档。

### Fixed
- 修复 `test_all` 系统测试使用固定 `/tmp` 文件名导致并行运行时互相污染的问题。
- 收紧 `StringUtils` 空指针 hex 转换、非法 hex、格式化空格式和严格 parse 边界行为。
- 修复 `Randomizer::randomBytes(nullptr, nonzero)` 可能崩溃的问题，并定义非法随机概率边界行为。

## [v2.1.3] - 2026-05-24

### Fixed
- 修复 `TomlParser` 解析多行 TOML 数组失败的问题，支持数组尾逗号、数组内注释和空行。
- 修复 TOML 数组字符串元素包含逗号时 `getArray()` 误拆分的问题，并加强字符串引号校验与 literal string 反斜杠处理。
- 修复 `defn.hpp` 与 `backtrace/trace.hpp` 中重复声明导致 `test_all` 无法编译的问题。
- 修复 `hmac.hpp` 单独包含时缺少 `<vector>` 依赖的问题。

### Docs
- 同步更新 `TomlParser` 文档，移除“仅支持单行数组”的过期描述。

## [v2.1.2] - 2026-05-20

### Docs
- 为所有头文件添加中文 Doxygen 文档注释，包括文件级、类/结构体级、方法级注释以及成员变量行尾注释

## [v2.1.1] - 2026-05-18

### Changed
- 将安装导出的 CMake targets 文件改为 `galayUtilsConfigTargets.cmake`，同步 package config 的 include 路径。
- Release 安装现在生成 `galayUtilsConfigTargets-release.cmake`，与新的驼峰导出文件命名保持一致。
- 将 CMake project 版本提升到 `2.1.1`，对齐本次发布 tag。

## [v2.1.0] - 2026-04-29

### Note
- Previous release. See git history for details.
