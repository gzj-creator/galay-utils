# galay-utils

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

一个以公开头文件、真实 target、真实示例与测试为准的 C++23 工具库文档入口。

## 仓库真相速览

| 项目 | 当前真实状态 |
|---|---|
| C++ 标准 | `CMAKE_CXX_STANDARD 23` |
| 基线 CMake | `3.16+` |
| 导出 target | `galay-utils`（INTERFACE）；满足条件时额外生成 `galay-utils-modules` |
| 真实示例 | `E1-BasicUsage`；满足模块条件时额外生成 `E1-BasicUsageImport` |
| 真实 benchmark | 当前无受版本控制的 benchmark 目录或 target |
| 真实测试 | `test/test_all.cpp` → target `test_all`；可选模块烟雾测试 `test_import_smoke` |
| Unix 链接依赖 | `pthread`；Linux 额外需要 `dl` |
| 额外头依赖 | 无 `galay-kernel` / `concurrentqueue` 依赖；限流器仅提供同步 `tryAcquire` 接口 |
| 文档真相来源 | 公开头文件 → 实现行为 → `examples/` → `test/` → Markdown |

## 模块概览

- 核心工具：`StringUtils`、`Randomizer`、`System`、`BackTrace`、`SignalHandler`
- 并发与资源：`ThreadPool`、`TaskWaiter`、`ThreadSafeList<T>`、`ObjectPool<T>`、`BlockingObjectPool<T>`
- 流控与容错：`CountingSemaphore`、`TokenBucketLimiter`、`SlidingWindowLimiter`、`LeakyBucketLimiter`、`CircuitBreaker`
- 路由与分布式：`RoundRobinLoadBalancer<T>`、`WeightRoundRobinLoadBalancer<T>`、`RandomLoadBalancer<T>`、`WeightedRandomLoadBalancer<T>`、`ConsistentHash`
- 数据结构：`TrieTree`、`Mvcc<T>`、`Snapshot`、`Transaction<T>`、`Huffman*`
- 应用与系统：`App` / `Cmd` / `Arg`、`ConfigParser`、`IniParser`、`EnvParser`、`TomlParser`、`ParserManager`、`Process`
- 算法：`Base64Util`、`MD5Util`、`MurmurHash3Util`、`SaltGenerator`、`SHA256`、`HMAC`

## 快速开始

### 1. 仅验证基础配置

前置条件：

- 编译器支持 C++23
- CMake `>= 3.16`
- 如果你只想验证头文件布局，保持默认 `BUILD_MODULE_TESTS=OFF` 即可；只有要验证模块导入时再显式开启

```bash
git clone https://github.com/gzj-creator/galay-utils.git
cd galay-utils

cmake -S . -B build \
  -DBUILD_MODULE_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTING=OFF

cmake --build build --parallel
```

### 2. 构建真实示例 `E1-BasicUsage`

前置条件：

- 编译器支持 C++23
- CMake `>= 3.16`
- `examples/include/e1_basic.cpp` 使用 umbrella header `galay-utils/galay_utils.hpp`
- 当前 umbrella header 不再默认导出 `RateLimiter`
- 限流器头文件也不再要求 `galay-kernel` / `concurrentqueue` 头文件

```bash
cmake -S . -B build-examples \
  -DBUILD_MODULE_TESTS=OFF \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTING=OFF

cmake --build build-examples --target E1-BasicUsage --parallel
./build-examples/examples/E1-BasicUsage
```

示例真相来源：

- 源文件：`examples/include/e1_basic.cpp`
- target：`E1-BasicUsage`
- 环境变量：无

### 3. 构建真实测试

前置条件：

- 编译器支持 C++23
- CMake `>= 3.16`
- `test/test_all.cpp` 包含 umbrella header，并额外直接包含 `ratelimiter/limiter.hpp`
- 无需安装 `galay-kernel` 或 `concurrentqueue`

```bash
cmake -S . -B build-test \
  -DBUILD_MODULE_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTING=ON

cmake --build build-test --target test_all --parallel
ctest --test-dir build-test --output-on-failure
```

说明：

- `test/CMakeLists.txt` 只链接 `galay-utils`
- `ctest --test-dir build-test` 不需要额外 loader 环境变量

### 4. 构建 C++23 模块烟雾测试

前置条件：

- CMake `>= 3.28`
- `Ninja` 或 `Visual Studio` 生成器
- 非 `AppleClang`
- 可用的 C++23 模块扫描工具链
- 当前模块烟雾测试只验证 `StringUtils` 与 `System` 的导入面

```bash
cmake -S . -B build-mod -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DBUILD_TESTING=ON \
  -DBUILD_MODULE_TESTS=ON \
  -DBUILD_EXAMPLES=OFF

cmake --build build-mod --target test_import_smoke --parallel
ctest --test-dir build-mod -R ModuleImportSmoke --output-on-failure
```

## 集成方式

### CMake 子目录

```cmake
add_subdirectory(third_party/galay-utils)
target_link_libraries(your_target PRIVATE galay-utils)
```

### 已安装包

```cmake
find_package(galay-utils REQUIRED)
target_link_libraries(your_target PRIVATE galay::galay-utils)
```

### 头文件选择建议

- 只用字符串/系统/随机等轻量模块时，优先直接包含对应头文件，例如 `galay-utils/string/string.hpp`
- 只有在需要限流器家族时，再额外包含 `galay-utils/ratelimiter/limiter.hpp`
- 只有在工具链满足模块要求时，再使用 `import galay.utils;`

## 依赖边界

- `galay-utils` 是接口库，公开头不再依赖 `galay-kernel`
- `galay-utils/ratelimiter/limiter.hpp` 仅保留同步非阻塞 `tryAcquire` 路径
- 异步限流器和 `acquire()` awaitable 已移除；部分限流器内部带锁，不适合作为协程调度器上的 awaitable 使用
- `test/test_all.cpp` 与 `test/import_smoke.cpp` 不再额外链接 `galay-kernel`

## 文档导航

- [文档首页](docs/README.md)
- [00-快速开始](docs/00-快速开始.md)
- [01-架构设计](docs/01-架构设计.md)
- [02-API参考](docs/02-API参考.md)
- [03-使用指南](docs/03-使用指南.md)
- [04-示例代码](docs/04-示例代码.md)
- [05-性能测试](docs/05-性能测试.md)
- [06-高级主题](docs/06-高级主题.md)
- [07-常见问题](docs/07-常见问题.md)

历史专题页已经收敛进编号文档，避免同一概念在多个页面重复出现并干扰 RAG 检索。当前官方导航只保留上面的 canonical 文档集合。

## 已知限制

- 当前仓库没有受版本控制的 `benchmark/` 目录；旧文档中的性能页仅保留“当前无 benchmark 资产”的边界说明
- 默认 `BUILD_MODULE_TESTS=OFF`；验证模块导入时再显式传 `-DBUILD_MODULE_TESTS=ON`
- 四个 `LoadBalancer` 的 `select()` 都返回 `std::optional<T>` 按值，不属于零拷贝 API
- 只有 `RoundRobinLoadBalancer<T>::select()` 在节点集合不再变更时适合并发共享；并发 `append()` 以及其余三种 balancer 的共享实例都需要外部同步

## 许可

MIT
