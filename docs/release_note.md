# Release Note

按时间顺序追加版本记录，避免覆盖历史发布说明。

## v1.2.1 - 2026-04-23

- 版本级别：小版本（patch）
- Git 提交消息：`chore: 发布 v1.2.1`
- Git Tag：`v1.2.1`
- 自述摘要：
  - 将源码中的项目版本提升到 `1.2.1`，使 `galay-utils` 的 CMake 版本元数据与本次发布 tag 保持一致。
  - 延续 `galay-utils-config-version.cmake` 的生成与安装链路，修复下游执行带版本约束的 `find_package(galay-utils ... CONFIG)` 时命中旧 tag 后无法完成版本匹配的问题。
  - 新增 `scripts/tests/test_cmake_packaging.sh`，回归验证安装后的版本化 CMake 包消费路径。

## v2.0.0 - 2026-04-29

- 版本级别：大版本（major）
- Git 提交消息：`refactor: 统一源码文件命名规范`
- Git Tag：`v2.0.0`
- 自述摘要：
  - 将源码、头文件、测试、示例与 benchmark 文件统一重命名为 lower_snake_case，编号前缀同步改为小写下划线形式。
  - 同步更新 CMake/Bazel 构建描述、模块入口、README/docs、脚本和所有项目内 include 路径引用。
  - 移除项目内相对 include，统一使用基于公开 include 根或模块根的非相对路径。

## v2.1.0 - 2026-04-29

- 版本级别：中版本（minor）
- Git 提交消息：`refactor: 移除异步限流器并发布 v2.1.0`
- Git Tag：`v2.1.0`
- 自述摘要：
  - 新增 `IniParser`、`TomlParser` 与 `parser_manager.hpp` 统一出口，`ParserManager` 默认支持 `.conf`、`.ini`、`.env`、`.toml`。
  - 拆分 parser 模块头文件，保留 `parser.hpp` 兼容入口，并同步更新 umbrella header、C++ 模块入口与使用文档。
  - 移除 `RateLimiter` 的异步 `acquire()` / awaitable 路径，只保留同步非阻塞 `tryAcquire()` 接口。
  - 移除限流器对 `galay-kernel` 与 `concurrentqueue/moodycamel` 的依赖，测试构建不再查找或链接 `galay-kernel`。
  - 补充 TOML 异常解析测试与限流器正确性/多线程压测，覆盖常见非法 TOML、容量超限、速率恢复和并发获取路径。
