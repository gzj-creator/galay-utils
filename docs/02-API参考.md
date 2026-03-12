# 02-API参考

本页按公开头文件整理当前可见 API。若文档与源码冲突，以 `galay-utils/*.hpp` 为准。

## 1. 公开入口

| 入口 | 真实文件 / target | 说明 |
|---|---|---|
| 细粒度头文件 | `galay-utils/<module>/*.hpp` | 推荐作为最小依赖接入面 |
| umbrella header | `galay-utils/galay-utils.hpp` | 聚合所有公开头，也引入 `RateLimiter` 的外部依赖 |
| C++23 模块 | `galay-utils/module/galay.utils.cppm` | 通过 `import galay.utils;` 导入，导出面与 umbrella 基本一致 |

完整公开头文件清单：

- `galay-utils/galay-utils.hpp`
- `galay-utils/string/String.hpp`
- `galay-utils/random/Random.hpp`
- `galay-utils/system/System.hpp`
- `galay-utils/common/TypeName.hpp`
- `galay-utils/backtrace/BackTrace.hpp`
- `galay-utils/signal/SignalHandler.hpp`
- `galay-utils/thread/Thread.hpp`
- `galay-utils/pool/Pool.hpp`
- `galay-utils/ratelimiter/RateLimiter.hpp`
- `galay-utils/circuitbreaker/CircuitBreaker.hpp`
- `galay-utils/balancer/LoadBalancer.hpp`
- `galay-utils/consistent_hash/ConsistentHash.hpp`
- `galay-utils/trie/TrieTree.hpp`
- `galay-utils/mvcc/Mvcc.hpp`
- `galay-utils/huffman/Huffman.hpp`
- `galay-utils/args/App.hpp`
- `galay-utils/parser/Parser.hpp`
- `galay-utils/process/Process.hpp`
- `galay-utils/algorithm/Base64.hpp`
- `galay-utils/algorithm/MD5.hpp`
- `galay-utils/algorithm/MurmurHash3.hpp`
- `galay-utils/algorithm/Salt.hpp`
- `galay-utils/algorithm/HMAC.hpp`
- `galay-utils/common/Defn.hpp`
- `galay-utils/module/ModulePrelude.hpp`
- `galay-utils/module/galay.utils.cppm`

## 2. 核心工具

| 模块 | 头文件 | 主要类型 / 函数 |
|---|---|---|
| String | `galay-utils/string/String.hpp` | `StringUtils` |
| Random | `galay-utils/random/Random.hpp` | `Randomizer` |
| System | `galay-utils/system/System.hpp` | `System`、`System::AddressType` |
| TypeName | `galay-utils/common/TypeName.hpp` | `getTypeName<T>()`、`getTypeName(obj)`、`demangleSymbol()` |
| BackTrace | `galay-utils/backtrace/BackTrace.hpp` | `BackTrace` |
| Signal | `galay-utils/signal/SignalHandler.hpp` | `SignalHandler` |

### `StringUtils`

- `split(std::string_view, char)`
- `split(std::string_view, std::string_view)`
- `splitRespectQuotes(std::string_view, char, char)`
- `join(const std::vector<std::string>&, std::string_view)`
- `trim` / `trimLeft` / `trimRight`
- `toLower` / `toUpper`
- `startsWith` / `endsWith` / `contains`
- `replace` / `replaceFirst`
- `count(char)` / `count(std::string_view)`
- `toHex` / `fromHex` / `toVisibleHex`
- `isInteger` / `isFloat` / `isBlank`
- `format(...)`
- `parse<T>(...)`
- `toString(...)`

### `Randomizer`

- `static Randomizer& instance()`
- `randomInt` / `randomUint32` / `randomUint64`
- `randomDouble` / `randomFloat` / `randomBool`
- `randomString` / `randomHex` / `randomBytes`
- `uuid()`
- `seed()` / `reseed()`

### `System`

- 时间：`System::currentTimeMs` / `System::currentTimeUs` / `System::currentTimeNs`
- 时间格式：`System::currentGMTTime` / `System::currentLocalTime` / `System::formatTime`
- 文件：`System::readFile` / `System::writeFile` / `System::readFileMmap`
- 文件系统：`System::fileExists` / `System::isDirectory` / `System::fileSize` / `System::createDirectory` / `System::remove` / `System::listDirectory`
- 环境变量：`System::getEnv` / `System::setEnv` / `System::unsetEnv`
- 网络：`System::resolveHostIPv4` / `System::resolveHostIPv6` / `System::checkAddressType`
- 主机：`System::cpuCount` / `System::hostname` / `System::currentDir` / `System::changeDir` / `System::executablePath`

### `TypeName`

- `template<typename T> getTypeName() -> std::string`
- `template<typename T> getTypeName(const T& obj) -> std::string`
- `demangleSymbol(const char* mangledName) -> std::string`
- 语义：GCC / Clang 下会尝试 demangle；失败或平台不支持时返回原始 `typeid(...).name()` / 符号名；`nullptr` 输入返回空字符串

### `BackTrace`

- `getStackTrace(int maxFrames = 64, int skipFrames = 1) -> std::vector<std::string>`
- `printStackTrace(int maxFrames = 64, int skipFrames = 1)`
- `getStackTraceString(int maxFrames = 64, int skipFrames = 1) -> std::string`
- `installCrashHandlers()`
- 语义：
  - 当前只在 `__APPLE__` / `__linux__` 上真正采集堆栈；其他平台会返回空栈或只输出 `0 frames`
  - `printStackTrace(...)` / `getStackTraceString(...)` 会在传入的 `skipFrames` 基础上再额外跳过 1 帧，用来隐藏包装函数自身
  - `installCrashHandlers()` 会安装 `SIGSEGV` / `SIGABRT` / `SIGFPE` / `SIGILL`，在支持的平台上额外安装 `SIGBUS`
  - crash handler 打印堆栈后会把该 signal 的处理方式恢复为 `SIG_DFL`，再重新 `raise(signal)`，因此进程仍会按默认方式终止

### `SignalHandler`

- `using Handler = std::function<void(int)>`
- `static SignalHandler& instance()`
- `bool setHandler(int signal, Handler handler)`
- `template<int... Signals> bool setHandler(Handler handler)`
- `bool removeHandler(int signal)` / `bool restoreDefault(int signal)`
- `bool ignoreSignal(int signal)`
- `bool blockSignal(int signal)` / `bool unblockSignal(int signal)`
- `bool hasHandler(int signal) const`
- 语义：
  - `setHandler(...)` / `removeHandler(...)` / `restoreDefault(...)` / `ignoreSignal(...)` / `blockSignal(...)` / `unblockSignal(...)` 都返回 `bool`
  - Windows 下 `setHandler(...)` / `removeHandler(...)` / `ignoreSignal(...)` 走 `std::signal(...)`；`blockSignal()` / `unblockSignal()` 固定返回 `false`
  - POSIX 下 `setHandler(...)` 使用 `sigaction(..., SA_RESTART, ...)` 注册进程级 signal handler
  - `blockSignal(...)` / `unblockSignal(...)` 在 POSIX 下通过 `pthread_sigmask(...)` 修改的是当前线程的 signal mask，而不是全局进程 mask

## 3. 并发与资源

| 模块 | 头文件 | 主要类型 |
|---|---|---|
| Thread | `galay-utils/thread/Thread.hpp` | `ThreadPool`、`TaskWaiter`、`ThreadSafeList<T>` |
| Pool | `galay-utils/pool/Pool.hpp` | `PoolableObject`、`ObjectPool<T>`、`BlockingObjectPool<T>` |

### `ThreadPool`

- `ThreadPool(size_t numThreads = 0)`
- `addTask(F&&, Args&&...) -> std::future<std::invoke_result_t<F, Args...>>`
- `execute(F&&)`
- `threadCount()` / `pendingTasks()` / `isStopped()`
- `waitAll()`
- `stop()` / `stopNow()`
- 语义：`addTask(...)` 在池已停止时抛 `std::runtime_error`；`execute(...)` 只派发任务，不返回 `future`

### `TaskWaiter`

- `addTask(ThreadPool&, F&&)`
- `wait()`
- `waitFor(timeout)`

### `ThreadSafeList<T>`

- `pushFront` / `pushBack`
- `popFront` / `popBack`
- `remove(Node*)`
- `size()` / `empty()` / `clear()`

### `PoolableObject` / `IsPoolable<T>`

- `virtual ~PoolableObject() = default`
- `virtual void reset()`
- `IsPoolable<T>`：`T` 继承 `PoolableObject`，或自行提供 `reset() -> void`

### `ObjectPool<T>`

- `using Ptr = std::unique_ptr<T, std::function<void(T*)>>`
- `using Creator = std::function<T*()>`
- `using Destroyer = std::function<void(T*)>`
- `ObjectPool(size_t initialSize = 0, size_t maxSize = 0, Creator creator = nullptr, Destroyer destroyer = nullptr)`
- `acquire()`：优先复用池内对象；池空时按需新建
- `tryAcquire()`：仅在池内已有对象时成功，否则返回空 `Ptr`
- `size()` / `totalCreated()` / `empty()`
- `clear()` / `shrink(size_t targetSize)`

### `BlockingObjectPool<T>`

- `using Ptr = std::unique_ptr<T, std::function<void(T*)>>`
- `using Creator = std::function<T*()>`
- `using Destroyer = std::function<void(T*)>`
- `BlockingObjectPool(size_t poolSize, Creator creator = nullptr, Destroyer destroyer = nullptr)`
- `acquire()`：阻塞直到池内有对象可取
- `tryAcquireFor(timeout)`：超时返回空 `Ptr`
- `available()`
- 语义：这是固定大小阻塞池；没有 `tryAcquire()`、`totalCreated()`、`clear()`、`shrink()` 这组 API

## 4. 流控与容错

| 模块 | 头文件 | 主要类型 |
|---|---|---|
| RateLimiter | `galay-utils/ratelimiter/RateLimiter.hpp` | `CountingSemaphore`、`TokenBucketLimiter`、`SlidingWindowLimiter`、`LeakyBucketLimiter` |
| CircuitBreaker | `galay-utils/circuitbreaker/CircuitBreaker.hpp` | `CircuitState`、`CircuitBreakerConfig`、`CircuitBreaker` |

### `RateLimiter`

`RateLimiter.hpp` 的公开面分为四个限流器类型，以及四个与 `acquire()` 路径配套的 awaitable 类型：

- `CountingSemaphore`
  - `tryAcquire(size_t n = 1)`
  - `acquire(size_t n = 1) -> SemaphoreAwaitable`
  - `release(size_t n = 1)`
  - `available()`
- `TokenBucketLimiter`
  - `TokenBucketLimiter(double rate, size_t capacity)`
  - `tryAcquire(size_t tokens = 1)`
  - `acquire(size_t tokens = 1) -> TokenBucketAwaitable`
  - `availableTokens()`
  - `setRate(double)` / `setCapacity(size_t)`
  - `rate()` / `capacity()`
- `SlidingWindowLimiter`
  - `SlidingWindowLimiter(size_t maxRequests, std::chrono::milliseconds windowSize)`
  - `tryAcquire()`
  - `acquire() -> SlidingWindowAwaitable`
  - `currentCount()`
  - `reset()`
- `LeakyBucketLimiter`
  - `LeakyBucketLimiter(double rate, size_t capacity)`
  - `tryAcquire(size_t amount = 1)`
  - `acquire(size_t amount = 1) -> LeakyBucketAwaitable`
  - `currentLevel()`
- `SemaphoreAwaitable`
  - `SemaphoreAwaitable(CountingSemaphore*, size_t)`
  - `await_ready() noexcept`
  - `await_suspend(std::coroutine_handle<>) noexcept`
  - `await_resume() noexcept -> std::expected<void, kernel::IOError>`
- `TokenBucketAwaitable`
  - `TokenBucketAwaitable(TokenBucketLimiter*, size_t)`
  - `await_ready() noexcept`
  - `await_suspend(std::coroutine_handle<>) noexcept`
  - `await_resume() noexcept -> std::expected<void, kernel::IOError>`
- `SlidingWindowAwaitable`
  - `SlidingWindowAwaitable(SlidingWindowLimiter*)`
  - `await_ready() noexcept`
  - `await_suspend(std::coroutine_handle<>) noexcept`
  - `await_resume() noexcept -> std::expected<void, kernel::IOError>`
- `LeakyBucketAwaitable`
  - `LeakyBucketAwaitable(LeakyBucketLimiter*, size_t)`
  - `await_ready() noexcept`
  - `await_suspend(std::coroutine_handle<>) noexcept`
  - `await_resume() noexcept -> std::expected<void, kernel::IOError>`

获取路径映射：

- `CountingSemaphore::acquire(...)` → `SemaphoreAwaitable`
- `TokenBucketLimiter::acquire(...)` → `TokenBucketAwaitable`
- `SlidingWindowLimiter::acquire()` → `SlidingWindowAwaitable`
- `LeakyBucketLimiter::acquire(...)` → `LeakyBucketAwaitable`

等待语义：

- 四个 awaitable 都继承 `kernel::TimeoutSupport<...>`，因此支持 `co_await limiter.acquire(...)`
- 若链式调用 `.timeout(...)`，恢复点的结果仍是 `std::expected<void, kernel::IOError>`，而不是 `bool`

依赖边界：

- 该头文件无条件依赖 `galay-kernel` 与 `concurrentqueue/moodycamel`
- 因此它既是功能 API，也是外部依赖边界

### `CircuitBreaker`

- `CircuitBreakerConfig`
  - `failureThreshold`
  - `successThreshold`
  - `resetTimeout`
- `CircuitBreaker`
  - `allowRequest()`
  - `onSuccess()` / `onFailure()`
  - `execute(F&&)`
  - `executeWithFallback(F&&, Fallback&&)`
  - `state()` / `stateString()`
  - `failureCount()` / `successCount()`
  - `reset()` / `forceOpen()`
  - `config()`

## 5. 路由、分布式与数据结构

| 模块 | 头文件 | 主要类型 / 方法 |
|---|---|---|
| Balancer | `galay-utils/balancer/LoadBalancer.hpp` | `RoundRobinLoadBalancer<T>`、`WeightRoundRobinLoadBalancer<T>`、`RandomLoadBalancer<T>`、`WeightedRandomLoadBalancer<T>` |
| ConsistentHash | `galay-utils/consistent_hash/ConsistentHash.hpp` | `NodeConfig`、`NodeStatus`、`PhysicalNode`、`ConsistentHash` |
| Trie | `galay-utils/trie/TrieTree.hpp` | `TrieTree` |
| MVCC | `galay-utils/mvcc/Mvcc.hpp` | `VersionedValue<T>`、`Mvcc<T>`、`Snapshot`、`Transaction<T>` |
| Huffman | `galay-utils/huffman/Huffman.hpp` | `HuffmanCode`、`HuffmanTable<T>`、`HuffmanEncoder<T>`、`HuffmanDecoder<T>`、`HuffmanBuilder<T>` |

### `Balancer`

- `RoundRobinLoadBalancer<T>`：`select()` / `size()` / `append(Type)`
- `WeightRoundRobinLoadBalancer<T>`：`select()` / `size()` / `append(Type, uint32_t)`
- `RandomLoadBalancer<T>`：`select()` / `size()` / `append(Type)`
- `WeightedRandomLoadBalancer<T>`：`select()` / `size()` / `append(Type, uint32_t)`

### `ConsistentHash`

- `NodeStatus`
  - 数据成员：`healthy` / `requestCount` / `failureCount`
  - `recordRequest()` / `recordFailure()`
  - `markHealthy()` / `reset()`
- `NodeConfig`
  - 数据成员：`id` / `endpoint` / `weight = 1`
  - `operator==(const NodeConfig&)`
- `PhysicalNode`
  - 数据成员：`config` / `status`
  - `explicit PhysicalNode(NodeConfig cfg)`
- `ConsistentHash`
  - `using HashFunc = std::function<uint32_t(const std::string&)>`
  - `ConsistentHash(size_t virtualNodes = 150, HashFunc hashFunc = nullptr)`
  - `addNode(const NodeConfig&)`
  - `removeNode(const std::string& nodeId)`
  - `getNode(const std::string& key) -> std::optional<NodeConfig>`
  - `getHealthyNode(const std::string& key, size_t maxRetries = 3) -> std::optional<NodeConfig>`
  - `getNodes(const std::string& key, size_t count) -> std::vector<NodeConfig>`
  - `markUnhealthy(const std::string&)` / `markHealthy(const std::string&)`
  - `getAllNodes() -> std::vector<NodeConfig>`
  - `nodeCount()` / `virtualNodeCount()` / `empty()` / `clear()`
- 语义：当前公开头里没有 `getNodeStatus()`；状态相关检索应落到 `NodeStatus`、`PhysicalNode` 以及 `markHealthy()` / `markUnhealthy()`

### `TrieTree`

- `add`
- `contains`
- `startsWith`
- `query`
- `remove`
- `getWordsWithPrefix`
- `getAllWords`
- `size()` / `empty()` / `clear()`

### `MVCC`

- `using Version = uint64_t`
- `VersionedValue<T>`
  - 数据成员：`version` / `value` / `deleted`
  - `VersionedValue(Version v, std::unique_ptr<T> val, bool del = false)`
- `Mvcc<T>`
  - `Mvcc()`
  - `getValue(Version version) const -> const T*`
  - `getCurrentValue() const -> const T*`
  - `getValueWithVersion(Version version) const -> std::pair<const T*, Version>`
  - `putValue(std::unique_ptr<T>) -> Version`
  - `putValue(const T&) -> Version`
  - `updateValue(std::function<std::unique_ptr<T>(const T*)>) -> Version`
  - `compareAndSwap(Version expectedVersion, std::unique_ptr<T> newValue) -> Version`
  - `removeValue(Version version)` / `deleteValue() -> Version`
  - `isValid(Version version) const`
  - `currentVersion() const` / `versionCount() const`
  - `gc(size_t keepVersions)` / `gcOlderThan(Version olderThan)`
  - `getAllVersions() const -> std::vector<Version>`
  - `clear()`
- `Snapshot`
  - `explicit Snapshot(Version version)`
  - `version() const`
  - `read(const Mvcc<T>& mvcc) const -> const T*`
- `Transaction<T>`
  - `explicit Transaction(Mvcc<T>& mvcc)`
  - `read() const -> const T*`
  - `write(std::unique_ptr<T> value)`
  - `commit()`
  - `isCommitted() const`
- 语义：`compareAndSwap(...)` 冲突时返回 `0`；`deleteValue()` 会写入 tombstone 版本，而不是立即擦除历史版本

### `Huffman`

- `HuffmanCode`
  - 数据成员：`code` / `length`
- `HuffmanTable<T>`
  - `HuffmanTable()`
  - `addCode(const T& symbol, uint32_t code, uint8_t length)`
  - `getCode(const T& symbol) const -> const HuffmanCode&`
  - `hasSymbol(const T& symbol) const`
  - `getSymbol(uint32_t code, uint8_t length) const -> const T&`
  - `tryGetSymbol(uint32_t code, uint8_t length, T& symbol) const`
  - `getSymbols() const -> std::vector<T>`
  - `size() const`
  - `clear()`
- `HuffmanEncoder<T>`
  - `explicit HuffmanEncoder(const HuffmanTable<T>& table)`
  - `encode(const T& symbol)`
  - `encode(const std::vector<T>& symbols)`
  - `finish() -> std::vector<uint8_t>`
  - `bitCount() const`
  - `reset()`
- `HuffmanDecoder<T>`
  - `HuffmanDecoder(const HuffmanTable<T>& table, uint8_t minCodeLen = 1, uint8_t maxCodeLen = 32)`
  - `decode(const std::vector<uint8_t>& data, size_t symbolCount = 0) -> std::vector<T>`
- `HuffmanBuilder<T>`
  - `build(const std::unordered_map<T, size_t>& frequencies) -> HuffmanTable<T>`
  - `buildFromData(const std::vector<T>& data) -> HuffmanTable<T>`
- 语义：`HuffmanTable<T>::getCode()` / `getSymbol()` 在缺失项上抛异常；`HuffmanDecoder<T>::decode()` 在超过 `maxCodeLen` 时抛 `std::runtime_error`

## 6. 应用与系统集成

| 模块 | 头文件 | 主要类型 |
|---|---|---|
| App | `galay-utils/args/App.hpp` | `ArgType`、`ArgValue`、`Arg`、`Cmd`、`App` |
| Parser | `galay-utils/parser/Parser.hpp` | `ParserBase`、`ConfigParser`、`EnvParser`、`ParserManager` |
| Process | `galay-utils/process/Process.hpp` | `ProcessId`、`ExitStatus`、`Process` |

### `App`

- `ArgType`
  - `Bool` / `Int` / `Float` / `Double` / `String`
- `ArgValue`
  - `using Value = std::variant<bool, int, float, double, std::string>`
  - `ArgValue()`
  - `ArgValue(Value)`
  - `as<T>()`
  - `isSet()`
  - `set(Value)`
- `Arg`
  - `Arg()`
  - `Arg(std::string longName, std::string description)`
  - `shortName(char)`
  - `type(ArgType)`
  - `required(bool = true)`
  - `defaultValue(...)`
  - `flag(bool = true)`
  - `longName()` / `shortName()` / `description()`
  - `type()` / `isRequired()` / `isFlag()` / `defaultValue()`
- `Cmd`
  - `addArg(Arg)`
  - `addSubcommand(std::unique_ptr<Cmd>)`
  - `callback(Callback)`
  - `get(name)` / `getAs<T>(name)` / `has(name)`
  - `positional()`
  - `name()` / `description()`
  - `printHelp()`
- `App`
  - `App(std::string name, std::string description = "")`
  - `run(int argc, char* argv[])`

### `Parser`

- `ParserBase`
  - `parseFile`
  - `parseString`
  - `getValue`
  - `hasKey`
  - `getKeys`
  - `getValueAs<T>`
  - `lastError()`
- `ConfigParser`
  - `getKeysInSection`
  - `getArray`
- `EnvParser`
  - 继承 `ParserBase`
- `ParserManager`
  - `instance()`
  - `registerParser(extension, creator)`
  - `createParser(path)`
  - 默认注册：`.conf` / `.ini` → `ConfigParser`，`.env` → `EnvParser`

### `Process`

- `ProcessId`
  - Windows：`DWORD`
  - POSIX：`pid_t`
- `ExitStatus`
  - 数据成员：`code` / `signaled` / `signal`
  - `success() const`
- `Process`
  - `Process::currentId()`
  - `Process::parentId()`
  - `wait(ProcessId pid, int options = 0) -> std::optional<ExitStatus>`
  - `spawn(const std::string& path, const std::vector<std::string>& args) -> ProcessId`
  - `execute(const std::string& command) -> ExitStatus`
  - `executeWithOutput(const std::string& command) -> std::pair<ExitStatus, std::string>`
  - `kill(ProcessId pid, int signal)`
  - `Process::isRunning(ProcessId pid)`
  - `daemonize()`

## 7. 算法与辅助模块

| 头文件 | 主要类型 |
|---|---|
| `galay-utils/algorithm/Base64.hpp` | `Base64Util` |
| `galay-utils/algorithm/MD5.hpp` | `MD5Util` |
| `galay-utils/algorithm/MurmurHash3.hpp` | `MurmurHash3Util` |
| `galay-utils/algorithm/Salt.hpp` | `SaltGenerator` |
| `galay-utils/algorithm/HMAC.hpp` | `SHA256`、`HMAC` |
| `galay-utils/common/Defn.hpp` | 基础类型别名、`NonCopyable`、`NonMovable`、`Singleton<T>` |

### `Base64Util`

- `Base64Encode(const std::string&, bool url = false)`
- `Base64EncodePem(const std::string&)`
- `Base64EncodeMime(const std::string&)`
- `Base64Decode(const std::string&, bool remove_linebreaks = false)`
- `Base64Encode(const unsigned char*, size_t len, bool url = false)`
- C++17：`Base64EncodeView(std::string_view, bool = false)`、`Base64EncodePemView(std::string_view)`、`Base64EncodeMimeView(std::string_view)`、`Base64DecodeView(std::string_view, bool = false)`
- 语义：
  - `url = false` 使用标准 Base64 字母表 `+/`；`url = true` 使用 URL-safe 字母表 `-_`
  - `Base64EncodePem(...)` 会按每 64 个字符插入换行；`Base64EncodeMime(...)` 会按每 76 个字符插入换行
  - `Base64Decode(...)` / `Base64DecodeView(...)` 遇到非法字符会抛 `std::runtime_error`
  - `remove_linebreaks = true` 会先移除输入中的 `
` 再解码，适合处理 PEM / MIME 风格输出

### `MD5Util`

- `MD5(const std::string&)` / `MD5(const unsigned char*, size_t)`
- `MD5Raw(const std::string&)` / `MD5Raw(const unsigned char*, size_t)`
- C++17：`MD5View(std::string_view)` / `MD5RawView(std::string_view)`
- 语义：`MD5(...)` / `MD5View(...)` 返回 32 字符小写十六进制字符串；`MD5Raw(...)` / `MD5RawView(...)` 返回 `std::array<uint8_t, 16>` 原始摘要字节

### `MurmurHash3Util`

- `Hash32(const void*, size_t, uint32_t seed = 0)` / `Hash32(const std::string&, uint32_t seed = 0)`
- `Hash128(const void*, size_t, uint32_t seed = 0)` / `Hash128(const std::string&, uint32_t seed = 0)`
- `Hash128Raw(const void*, size_t, uint32_t seed = 0)` / `Hash128Raw(const std::string&, uint32_t seed = 0)`
- C++17：`Hash32View(std::string_view, uint32_t seed = 0)`、`Hash128View(std::string_view, uint32_t seed = 0)`、`Hash128RawView(std::string_view, uint32_t seed = 0)`
- 语义：`Hash32(...)` 返回 32 位整数；`Hash128(...)` / `Hash128View(...)` 返回 32 字符十六进制字符串；`Hash128Raw(...)` / `Hash128RawView(...)` 返回 `std::array<uint64_t, 2>`

### `SaltGenerator`

- `generateHex(size_t length = 32)`
- `generateBase64(size_t length = 32)`
- `generateBytes(size_t length = 32)`
- `generateCustom(size_t length, const std::string& charset)`
- `generateSecureHex(size_t length = 32)`
- `generateSecureBase64(size_t length = 32)`
- `generateSecureBytes(size_t length = 32)`
- `generateBcryptSalt()`
- `generateTimestamped(size_t length = 32)`
- `isValidHex(const std::string&)` / `isValidBase64(const std::string&)`
- 语义：
  - `generateHex(length)` / `generateBase64(length)` / `generateBytes(length)` / `generateSecure*` 里的 `length` 表示“随机字节数”，不是最终字符串长度
  - 因而十六进制输出通常是 `2 * length` 个字符，Base64 输出通常接近 `4 * ceil(length / 3)` 个字符
  - `generateCustom(length, charset)` 的 `length` 才是最终输出字符数
  - `generateBcryptSalt()` 使用 16 个安全随机字节并输出 22 字符 bcrypt 风格 Base64 盐值

### `SHA256`

- `hash(const uint8_t* data, size_t length) -> std::array<uint8_t, 32>`
- `hashHex(const uint8_t* data, size_t length)`
- `hashHex(const std::string& data)`
- 语义：`hash(...)` 返回 32 字节原始摘要；`hashHex(...)` 返回 64 字符小写十六进制字符串

### `HMAC`

- `hmacSha256(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen) -> std::array<uint8_t, 32>`
- `hmacSha256(const std::string& key, const std::string& data) -> std::array<uint8_t, 32>`
- `hmacSha256Hex(const std::string& key, const std::string& data)`
- 语义：`hmacSha256(...)` 返回 32 字节原始 HMAC；`hmacSha256Hex(...)` 返回 64 字符小写十六进制字符串

### `Defn.hpp`

- 预处理宏：`GALAY_PLATFORM_*`、`GALAY_ARCH_*`、`GALAY_COMPILER_*`、`GALAY_LIKELY(x)`、`GALAY_UNLIKELY(x)`、`GALAY_FORCE_INLINE`、`GALAY_UNUSED(x)`
- 基础别名：`i8` / `i16` / `i32` / `i64`、`u8` / `u16` / `u32` / `u64`、`f32` / `f64`、`usize` / `isize`
- 指针与函数别名：`UniquePtr<T>`、`SharedPtr<T>`、`WeakPtr<T>`、`Func<T>`
- 字符串别名：`String` / `StringView`
- 基类：`NonCopyable`、`NonMovable`
- `Singleton<T>`
  - 继承：`NonCopyable`、`NonMovable`
  - `static T& instance()`

## 8. 已知 API 边界

- 文档中不再使用 `IniParser`；当前公开类型为 `ConfigParser`
- 文档中不再使用“API 索引”旧名；本页 canonical 标题为“API参考”
- 当前仓库现提供 `B1-CoreBench`，但本页仍不直接附带吞吐量表；性能口径统一以 [05-性能测试](05-性能测试.md) 为准

## 9. 返回、线程与使用语义

- 当前仓库没有统一的 `expected` / 错误码基类；检索失败语义时必须回到对应头文件签名，而不能把整个仓库当成单一错误模型
- 纯工具类主路径集中在 `string/`、`random/`、`algorithm/`、`common/`，它们主要回答“输入是什么、返回值是什么”
- 线程 / 资源相关能力集中在 `thread/`、`pool/`、`ratelimiter/`、`circuitbreaker/`、`process/`，检索时要额外关注阻塞/等待/资源释放语义
- `ThreadPool::addTask(...)` 返回 `std::future<...>`，而 `execute(...)` 是 fire-and-forget 风格；两者不应混用为同一等待模型
- `ObjectPool<T>` 与 `BlockingObjectPool<T>` 不是同一组方法：前者是“可扩容 + 非阻塞取对象”，后者是“固定池 + 阻塞等待”
- `RateLimiter::*::acquire(...)` 返回对应 `*Awaitable`，`co_await` 后的恢复值是 `std::expected<void, kernel::IOError>`，不是直接 `bool`
- `ConsistentHash` 当前没有 `getNodeStatus()` 公开 API；状态检索要从 `NodeStatus`、`PhysicalNode` 以及标记接口理解
- `App::run(...)`、`ParserBase::*`、`Process::*`、`System::*` 直接面向进程 / 文件系统 / 环境变量等外部状态，细节问题需要结合真实调用环境理解
- 资源生命周期主要集中在 `ThreadPool`、对象池、限流器、断路器与 `Process` 相关 API；纯字符串 / 哈希 / 编码工具通常是无状态或短生命周期值语义

## 10. 交叉验证入口

- 基础能力示例：`examples/include/E1-basic_usage.cpp`
- 系统 / 进程示例：`examples/include/E2-system_process.cpp`
- parser / balancer 示例：`examples/include/E3-parser_balancer.cpp`
- include / umbrella / 模块 smoke：`test/test_all.cpp`、`test/module_import_smoke.cpp`

## 11. 继续阅读

- [03-使用指南](03-使用指南.md)
- [04-示例代码](04-示例代码.md)
- [06-高级主题](06-高级主题.md)
