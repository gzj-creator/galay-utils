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
