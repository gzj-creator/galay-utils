# 02-API参考

本页按公开头文件整理当前可见 API。若文档与源码冲突，以 `galay-utils/*.hpp` 为准。

## 1. 公开入口

| 入口 | 真实文件 / target | 说明 |
|---|---|---|
| 细粒度头文件 | `galay-utils/<module>/*.hpp` | 推荐作为最小依赖接入面 |
| umbrella header | `galay-utils/galay_utils.hpp` | 聚合常用公开头，不默认导出 `RateLimiter` |
| C++23 模块 | `galay-utils/module/galay_utils.cppm` | 通过 `import galay.utils;` 导入，导出面与 umbrella 基本一致 |

完整公开头文件清单：

- `galay-utils/galay_utils.hpp`
- `galay-utils/string/string.hpp`
- `galay-utils/random/random.hpp`
- `galay-utils/system/system.hpp`
- `galay-utils/common/type_name.hpp`
- `galay-utils/backtrace/trace.hpp`
- `galay-utils/signal/signal.hpp`
- `galay-utils/thread/thread.hpp`
- `galay-utils/pool/pool.hpp`
- `galay-utils/ratelimiter/limiter.hpp`
- `galay-utils/circuitbreaker/breaker.hpp`
- `galay-utils/balancer/balancer.hpp`
- `galay-utils/consistent_hash/hash.hpp`
- `galay-utils/trie/trie.hpp`
- `galay-utils/mvcc/mvcc.hpp`
- `galay-utils/huffman/huffman.hpp`
- `galay-utils/args/app.hpp`
- `galay-utils/parser/parser_manager.hpp`
- `galay-utils/process/process.hpp`
- `galay-utils/algorithm/base64.hpp`
- `galay-utils/algorithm/md5.hpp`
- `galay-utils/algorithm/murmur_hash3.hpp`
- `galay-utils/algorithm/salt.hpp`
- `galay-utils/algorithm/hmac.hpp`
- `galay-utils/common/defn.hpp`
- `galay-utils/module/module_prelude.hpp`
- `galay-utils/module/galay_utils.cppm`

## 2. 核心工具

| 模块 | 头文件 | 主要类型 / 函数 |
|---|---|---|
| String | `galay-utils/string/string.hpp` | `StringUtils` |
| Random | `galay-utils/random/random.hpp` | `Randomizer` |
| System | `galay-utils/system/system.hpp` | `System`、`System::AddressType` |
| TypeName | `galay-utils/common/type_name.hpp` | `getTypeName<T>()`、`getTypeName(obj)`、`demangleSymbol()` |
| BackTrace | `galay-utils/backtrace/trace.hpp` | `BackTrace` |
| Signal | `galay-utils/signal/signal.hpp` | `SignalHandler` |

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
| Thread | `galay-utils/thread/thread.hpp` | `ThreadPool`、`TaskWaiter`、`ThreadSafeList<T>` |
| Pool | `galay-utils/pool/pool.hpp` | `PoolableObject`、`ObjectPool<T>`、`BlockingObjectPool<T>` |

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
| RateLimiter | `galay-utils/ratelimiter/limiter.hpp` | `CountingSemaphore`、`TokenBucketLimiter`、`SlidingWindowLimiter`、`LeakyBucketLimiter` |
| CircuitBreaker | `galay-utils/circuitbreaker/breaker.hpp` | `CircuitState`、`CircuitBreakerConfig`、`CircuitBreaker` |

### `RateLimiter`

`limiter.hpp` 的公开面分为四个同步非阻塞限流器类型；异步 `acquire()` / awaitable 路径已移除，避免引入 `galay-kernel`，也避免把内部带锁实现暴露给协程调度线程：

- `CountingSemaphore`
  - `tryAcquire(size_t n = 1)`
  - `release(size_t n = 1)`
  - `available()`
- `TokenBucketLimiter`
  - `TokenBucketLimiter(double rate, size_t capacity)`
  - `tryAcquire(size_t tokens = 1)`
  - `availableTokens()`
  - `setRate(double)` / `setCapacity(size_t)`
  - `rate()` / `capacity()`
- `SlidingWindowLimiter`
  - `SlidingWindowLimiter(size_t maxRequests, std::chrono::milliseconds windowSize)`
  - `tryAcquire()`
  - `maxRequests()`
  - `windowSize()`
- `LeakyBucketLimiter`
  - `LeakyBucketLimiter(double rate, size_t capacity)`
  - `tryAcquire(size_t amount = 1)`
  - `currentWater()`
  - `rate()` / `capacity()`

依赖边界：

- 该头文件仅依赖标准库
- 不再提供异步限流器，不再依赖 `galay-kernel` 或 `concurrentqueue/moodycamel`
- `SlidingWindowLimiter` 内部使用互斥锁保护窗口队列；不要把限流器当作 coroutine awaitable 使用

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
| Balancer | `galay-utils/balancer/balancer.hpp` | `RoundRobinLoadBalancer<T>`、`WeightRoundRobinLoadBalancer<T>`、`RandomLoadBalancer<T>`、`WeightedRandomLoadBalancer<T>` |
| ConsistentHash | `galay-utils/consistent_hash/hash.hpp` | `NodeConfig`、`NodeStatus`、`PhysicalNode`、`ConsistentHash` |
| Trie | `galay-utils/trie/trie.hpp` | `TrieTree` |
| MVCC | `galay-utils/mvcc/mvcc.hpp` | `VersionedValue<T>`、`Mvcc<T>`、`Snapshot`、`Transaction<T>` |
| Huffman | `galay-utils/huffman/huffman.hpp` | `HuffmanCode`、`HuffmanTable<T>`、`HuffmanEncoder<T>`、`HuffmanDecoder<T>`、`HuffmanBuilder<T>` |

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
| App | `galay-utils/args/app.hpp` | `ArgType`、`ArgValue`、`Arg`、`Cmd`、`App` |
| Parser | `galay-utils/parser/parser_manager.hpp` | `ParserBase`、`ConfigParser`、`IniParser`、`EnvParser`、`TomlParser`、`ParserManager` |
| Process | `galay-utils/process/process.hpp` | `ProcessId`、`ExitStatus`、`Process` |

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
- `IniParser`
  - 继承 `ConfigParser`
- `EnvParser`
  - 继承 `ParserBase`
- `TomlParser`
  - 继承 `ParserBase`
  - 支持基础 key-value、section、dotted key、字符串、数字、布尔值和单行数组
  - `getArray`
- `ParserManager`
  - `instance()`
  - `registerParser(extension, creator)`
  - `createParser(path)`
  - 默认注册：`.conf` → `ConfigParser`，`.ini` → `IniParser`，`.env` → `EnvParser`，`.toml` → `TomlParser`

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
| `galay-utils/algorithm/base64.hpp` | `Base64Util` |
| `galay-utils/algorithm/md5.hpp` | `MD5Util` |
| `galay-utils/algorithm/murmur_hash3.hpp` | `MurmurHash3Util` |
| `galay-utils/algorithm/salt.hpp` | `SaltGenerator` |
| `galay-utils/algorithm/hmac.hpp` | `SHA256`、`HMAC` |
| `galay-utils/common/defn.hpp` | 基础类型别名、`NonCopyable`、`NonMovable`、`Singleton<T>` |

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

### `defn.hpp`

- 预处理宏：`GALAY_PLATFORM_*`、`GALAY_ARCH_*`、`GALAY_COMPILER_*`、`GALAY_LIKELY(x)`、`GALAY_UNLIKELY(x)`、`GALAY_FORCE_INLINE`、`GALAY_UNUSED(x)`
- 基础别名：`i8` / `i16` / `i32` / `i64`、`u8` / `u16` / `u32` / `u64`、`f32` / `f64`、`usize` / `isize`
- 指针与函数别名：`UniquePtr<T>`、`SharedPtr<T>`、`WeakPtr<T>`、`Func<T>`
- 字符串别名：`String` / `StringView`
- 基类：`NonCopyable`、`NonMovable`
- `Singleton<T>`
  - 继承：`NonCopyable`、`NonMovable`
  - `static T& instance()`

## 8. 已知 API 边界

- `.ini` 使用独立公开类型 `IniParser`；`.toml` 使用 `TomlParser`
- 文档中不再使用“API 索引”旧名；本页 canonical 标题为“API参考”
- 当前仓库没有受版本控制的 benchmark target；性能页只保留“当前无 benchmark 资产”的边界说明

## 9. 返回、线程与使用语义

- 当前仓库没有统一的 `expected` / 错误码基类；检索失败语义时必须回到对应头文件签名，而不能把整个仓库当成单一错误模型
- 纯工具类主路径集中在 `string/`、`random/`、`algorithm/`、`common/`，它们主要回答“输入是什么、返回值是什么”
- 线程 / 资源相关能力集中在 `thread/`、`pool/`、`ratelimiter/`、`circuitbreaker/`、`process/`，检索时要额外关注阻塞/等待/资源释放语义
- `ThreadPool::addTask(...)` 返回 `std::future<...>`，而 `execute(...)` 是 fire-and-forget 风格；两者不应混用为同一等待模型
- `ObjectPool<T>` 与 `BlockingObjectPool<T>` 不是同一组方法：前者是“可扩容 + 非阻塞取对象”，后者是“固定池 + 阻塞等待”
- `RateLimiter` 不再提供 `acquire(...)` awaitable；使用 `tryAcquire(...)` 获取同步非阻塞结果
- `ConsistentHash` 当前没有 `getNodeStatus()` 公开 API；状态检索要从 `NodeStatus`、`PhysicalNode` 以及标记接口理解
- `App::run(...)`、`ParserBase::*`、`Process::*`、`System::*` 直接面向进程 / 文件系统 / 环境变量等外部状态，细节问题需要结合真实调用环境理解
- 资源生命周期主要集中在 `ThreadPool`、对象池、限流器、断路器与 `Process` 相关 API；纯字符串 / 哈希 / 编码工具通常是无状态或短生命周期值语义

## 10. 交叉验证入口

- 基础能力示例：`examples/include/e1_basic.cpp`
- import 示例：`examples/import/e1_basic.cpp`
- include / umbrella / 模块 smoke：`test/test_all.cpp`、`test/import_smoke.cpp`

## 11. 继续阅读

- [03-使用指南](03-使用指南.md)
- [04-示例代码](04-示例代码.md)
- [06-高级主题](06-高级主题.md)
