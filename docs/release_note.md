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
