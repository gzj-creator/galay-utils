# Changelog

维护说明：

- 本文件记录所有对用户有意义的变更，格式参考 Keep a Changelog，版本遵循 Semantic Versioning。
- 未发布提交先写入 `## [Unreleased]`；打 tag 时将累计条目收束为 `## [vX.Y.Z] - YYYY-MM-DD`。
- 版本级别按兼容性影响判定：破坏性变更升 major，新功能升 minor，修复、文档、配置维护升 patch。
- 内容粒度按 Added / Changed / Fixed / Docs / Chore 分组，只记录主要行为与接口变化，避免逐文件流水账。

## [Unreleased]

## [v3.2.0] - 2026-06-11

### Changed
- 将 `CircuitBreaker` 的 `execute()` 和 `executeWithFallback()` 从异常模型重构为 `std::expected` 模型：不再通过 `try/catch` 判断失败，改为检查返回结果的 `has_value()`；熔断打开时返回包含 `CircuitBreakerError::Open` 的失败结果，不抛异常。
- 引入 `CircuitBreakerExpected<T>` concept 约束执行接口接受的 expected-like 返回类型，通过编译期类型检查替代运行时异常。
- 将 `CircuitBreaker` 重命名为模板 `BasicCircuitBreaker<ClockType>`，支持注入自定义时钟源做确定性测试；`CircuitBreaker` 为默认 `steady_clock` 别名。
- 放宽部分原子操作的内存序（`acq_rel` → `relaxed`），减少不必要同步开销。
- 将超时时间预计算为纳秒整数，避免每次 `allowRequest()` 重复 `duration_cast`。
- 将 `TokenBucketLimiter`、`LeakyBucketLimiter` 的速率与容量改为原子定点状态，避免并发 setter 与 `tryAcquire()` 之间的数据竞争。
- 将 `SlidingWindowLimiter` 从 `std::mutex` + `std::deque` 改为固定原子槽位与 CAS 记录请求时间，限流器家族保持无锁同步非阻塞语义，未通过限流直接返回 `false`。

### Added
- 新增 `halfOpenMaxRequests` 配置字段，限制半开状态下同时放行的并发探测请求数。
- 新增配置归一化逻辑 `normalizeConfig()`，确保 `failureThreshold`、`successThreshold`、`halfOpenMaxRequests` 至少为 1，`resetTimeout` 非负。
- 新增 `forceOpen()` 时记录当前时间戳并重置计数器，使后续超时转换到半开状态语义正确。
- 新增 5 组 CircuitBreaker expected 执行、fallback、手动时钟超时、半开探测限流和 forceOpen 时间戳单测。
- 新增 `circuit_breaker_benchmark`，覆盖 Closed 成功/失败、Open 拒绝、HalfOpen 探测及多线程共享实例压测路径。
- 新增限流器源码无锁断言和 16 线程精确容量压测，覆盖 `CountingSemaphore`、`TokenBucketLimiter`、`SlidingWindowLimiter`、`LeakyBucketLimiter` 并发下不超放。

### Docs
- 更新 API 参考文档，补充 `CircuitBreakerError`、`CircuitBreakerExpected`、`BasicCircuitBreaker`、半开语义和配置归一化说明。
- 更新性能测试文档，补充 `circuit_breaker_benchmark` 条目。
- 更新 README、架构设计、API 参考、使用指南、高级主题和 FAQ，明确 RateLimiter 为无锁同步非阻塞接口，不提供 awaitable。

### Chore
- 将 CMake project 版本提升到 `3.2.0`，对齐本次中版本 tag。

## [v3.1.0] - 2026-06-07

### Added
- 新增 header-only `Bytes` 仅移动字节容器，支持 owning 深拷贝和 non-owning 字节视图。
- 新增 `ByteMetaData` 原始字节元数据和 `mallocBytes`、`deepCopyBytes`、`reallocBytes`、`clearBytes`、`freeBytes` 辅助函数。
- 将 `Bytes` 接入 umbrella header 与 C++23 module facade，并补充 buffer 测试和模块导入烟测覆盖。

### Docs
- 补充 `Bytes` 迁移设计与实现计划，明确 `galay-kernel` 后续删除本地 Bytes 实现并消费 `galay-utils`。
- 更新快速开始、API 参考、使用指南和 README 的缓存与缓冲工具说明。

### Chore
- 将 CMake project 版本提升到 `3.1.0`，对齐本次中版本 tag。

## [v3.0.0] - 2026-06-07

### Added
- 新增非线程安全的通用 `LruCache`，支持容量 LRU 淘汰、惰性 TTL 淘汰、自定义哈希/比较器、淘汰回调和可选 expire-after-access。
- 为 `LruCache` 增加可编译期开关控制的统计收集，默认关闭统计以减少容量缓存热路径开销。
- 增加 LRU 单测和 benchmark，对比 LeetCode 常见 LRU 实现、默认容量模式、统计开启模式和 TTL 模式。
- 新增轻量时间工具 `StopWatch`、`Deadline` 和 `Backoff`，保持无线程、无调度器、无阻塞语义。
- 新增 `ByteQueueView` 与 `RingBuffer` 通用缓冲工具，覆盖流式解析、环绕读写、span 视图和 POSIX `iovec` 视图。
- 增加 ByteQueueView、RingBuffer benchmark，并补充对应边界单测与压测文档。
- 新增 split-block `BloomFilter<T>`，用于高吞吐概率型存在性预过滤，并补充边界/压力测试与 benchmark。
- 新增本地无锁 `RandomGenerator`，用于协程热路径或单线程上下文，避免共享 `Randomizer` 的 mutex 竞争。
- 新增模块生产化路线图和目录收敛计划，并完成目录/测试布局收敛与 core 工具硬化两个阶段。

### Changed
- 将系统时间戳与时间格式化接口从 `System` 迁移到 `Time`，`System` 不再保留时间相关 API。
- 将公开头文件进一步收敛到 `process`、`tool`、`algorithm` 等 canonical include 目录，不保留兼容头。
- 将 `ThreadPool`、`TaskWaiter`、`ObjectPool`、`BlockingObjectPool` 迁入 `galay-utils/tool/`，移除旧 `galay-utils/concurrency/` 公开入口且不保留兼容头。
- 将 `LruCache`、`ByteQueueView`、`RingBuffer` 迁入 `galay-utils/cache/`，移除旧 `galay-utils/tool/` 缓存/缓冲公开入口且不保留兼容头。
- 将 `ThreadPool` 任务队列改为 moodycamel `BlockingConcurrentQueue`，提交路径不再使用 `std::mutex` / `std::condition_variable`，通过 CMake 暴露 `concurrentqueue` 头依赖，并移除无实际使用场景的 `ThreadSafeList` / `ListNode`。
- 将原单体 `test_all.cpp` 拆分为 `test/<area>/*_test.cpp` 分组 CTest target，并扩展模块导入烟测覆盖。
- 明确 `StringUtils`、`Time`、`TypeName`、`RandomGenerator`、`Randomizer` 的线程安全、阻塞与失败语义文档。

### Fixed
- 修复 `test_all` 系统测试使用固定 `/tmp` 文件名导致并行运行时互相污染的问题。
- 收紧 `StringUtils` 空指针 hex 转换、非法 hex、格式化空格式和严格 parse 边界行为。
- 修复 `Randomizer::randomBytes(nullptr, nonzero)` 可能崩溃的问题，并定义非法随机概率边界行为。

### Chore
- 将 CMake project 版本提升到 `3.0.0`，对齐本次主版本 tag。

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
