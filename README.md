# galay-utils

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

一个以公开头文件、真实 target、真实示例与测试为准的 C++23 工具库文档入口。

## 仓库真相速览

| 项目 | 当前真实状态 |
|---|---|
| C++ 标准 | `CMAKE_CXX_STANDARD 23` |
| 基线 CMake | `3.16+`，但若保留默认 `BUILD_MODULE_TESTS=ON`，配置阶段需要 `3.28+` |
| 导出 target | `galay-utils`（INTERFACE）；满足条件时额外生成 `galay-utils-modules` |
| 真实示例 | `E1-BasicUsage`、`E2-SystemProcess`、`E3-ParserBalancer` |
| 真实 benchmark | `benchmark/B1-core_benchmark.cpp` → target `B1-CoreBench` |
| 真实测试 | `test/test_all.cpp` → target `test_all`；可选模块烟雾测试 `test_import_smoke` |
| Unix 链接依赖 | `pthread`；Linux 额外需要 `dl` |
| 额外头依赖 | 使用 `galay-utils/galay-utils.hpp`、`galay-utils/ratelimiter/RateLimiter.hpp` 或 `import galay.utils;` 时，需要 `galay-kernel` 与 `concurrentqueue/moodycamel` 头文件 |
| 文档真相来源 | 公开头文件 → 实现行为 → `examples/` → `test/` → Markdown |

## 模块概览

- 核心工具：`StringUtils`、`Randomizer`、`System`、`BackTrace`、`SignalHandler`
- 并发与资源：`ThreadPool`、`TaskWaiter`、`ThreadSafeList<T>`、`ObjectPool<T>`、`BlockingObjectPool<T>`
- 流控与容错：`CountingSemaphore`、`TokenBucketLimiter`、`SlidingWindowLimiter`、`LeakyBucketLimiter`、`CircuitBreaker`
- 路由与分布式：`RoundRobinLoadBalancer<T>`、`WeightRoundRobinLoadBalancer<T>`、`RandomLoadBalancer<T>`、`WeightedRandomLoadBalancer<T>`、`ConsistentHash`
- 数据结构：`TrieTree`、`Mvcc<T>`、`Snapshot`、`Transaction<T>`、`Huffman*`
- 应用与系统：`App` / `Cmd` / `Arg`、`ConfigParser`、`EnvParser`、`ParserManager`、`Process`
- 算法：`Base64Util`、`MD5Util`、`MurmurHash3Util`、`SaltGenerator`、`SHA256`、`HMAC`

## 快速开始

### 1. 仅验证基础配置

前置条件：

- 编译器支持 C++23
- CMake `>= 3.16`
- 如果你只想验证头文件布局，建议显式关闭模块测试，避免默认 `BUILD_MODULE_TESTS=ON` 在 CMake `< 3.28` 时直接失败

```bash
git clone https://github.com/gzj-creator/galay-utils.git
cd galay-utils

cmake -S . -B build \
  -DBUILD_MODULE_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTS=OFF

cmake --build build --parallel
```

### 2. 构建真实示例 `E1-BasicUsage`

前置条件：

- 编译器支持 C++23
- CMake `>= 3.16`
- `examples/include/E1-basic_usage.cpp` 使用 umbrella header `galay-utils/galay-utils.hpp`
- 因此编译该示例时需要能找到 `galay-kernel` 和 `concurrentqueue/moodycamel` 头文件

```bash
cmake -S . -B build-examples \
  -DBUILD_MODULE_TESTS=OFF \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=OFF

cmake --build build-examples --target E1-BasicUsage --parallel
./build-examples/examples/E1-BasicUsage
```

示例真相来源：

- 源文件：`examples/include/E1-basic_usage.cpp`
- target：`E1-BasicUsage`
- 环境变量：无

### 3. 构建真实测试

前置条件：

- 编译器支持 C++23
- CMake `>= 3.16`
- `test/test_all.cpp` 直接包含 umbrella header
- `galay-kernel` 头文件与库可用；若未安装到默认前缀，请通过 `-DCMAKE_PREFIX_PATH=/path/to/prefix` 让 CMake 发现 `galay-kernel-config.cmake`

```bash
cmake -S . -B build-test \
  -DBUILD_MODULE_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTS=ON

cmake --build build-test --target test_all --parallel
ctest --test-dir build-test --output-on-failure
```

说明：

- `test/CMakeLists.txt` 现优先导入 `galay-kernel::galay-kernel`
- macOS 构建会把该共享库目录写入测试 target 的 `BUILD_RPATH`
- 因此在默认 `/usr/local` 安装下，`ctest --test-dir build-test` 不需要额外 `DYLD_LIBRARY_PATH`

### 4. 构建 C++23 模块烟雾测试

前置条件：

- CMake `>= 3.28`
- `Ninja` 或 `Visual Studio` 生成器
- 非 `AppleClang`
- 可用的 C++23 模块扫描工具链
- 仍然需要 `galay-kernel` / `concurrentqueue` 头文件，因为模块接口导出了 `RateLimiter.hpp`

```bash
cmake -S . -B build-mod -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DBUILD_TESTS=ON \
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

- 只用字符串/系统/随机等轻量模块时，优先直接包含对应头文件，例如 `galay-utils/string/String.hpp`
- 只有在确认具备 `galay-kernel` 与 `concurrentqueue` 头文件时，再使用 umbrella header `galay-utils/galay-utils.hpp`
- 只有在工具链满足模块要求时，再使用 `import galay.utils;`

## 依赖边界

- `galay-utils` 是接口库，但并不等于“所有入口都无外部依赖”
- `galay-utils/galay-utils.hpp` 会聚合 `RateLimiter.hpp`，而后者无条件包含 `galay-kernel` 与 `concurrentqueue/moodycamel`
- `RateLimiter` 的 `tryAcquire()` 与 `acquire()` 都在同一个公开头中，因此即使你只用非阻塞接口，编译该头仍需要上述外部头文件
- `test/test_all.cpp` 与 `test/module_import_smoke.cpp` 在 CMake 中还额外链接 `galay-kernel`

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

- 仓库现提供 `benchmark/` 与 `B1-CoreBench`；旧文档中的历史吞吐量表仍不被视为当前官方结果，应以源码和固定命令的本地输出为准
- 默认 `BUILD_MODULE_TESTS=ON` 会让 CMake `< 3.28` 的默认配置失败；基线构建请显式传 `-DBUILD_MODULE_TESTS=OFF`
- 四个 `LoadBalancer` 的 `select()` 都返回 `std::optional<T>` 按值，不属于零拷贝 API
- 只有 `RoundRobinLoadBalancer<T>::select()` 在节点集合不再变更时适合并发共享；并发 `append()` 以及其余三种 balancer 的共享实例都需要外部同步

## 许可

MIT
