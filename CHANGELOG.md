# CHANGELOG

维护说明：
- 未打 tag 的改动先写入 `

## [Unreleased]

## [v2.0.0] - 2026-04-29

### Changed
- 统一源码、头文件、测试、示例与 benchmark 文件命名为 `lower_snake_case`，编号前缀同步使用 `t<number>_`、`e<number>_` 与 `b<number>_` 风格。
- 同步更新构建脚本、模块入口、示例、测试、文档与脚本中的文件路径引用。
- 将项目内头文件包含调整为基于公开 include 根或模块根的非相对路径。

### Release
- 按大版本发布要求提升版本到 `v2.0.0`。

## [v1.2.1] - 2026-04-23

### Fixed
- 将 `CMakeLists.txt` 中的项目版本提升到 `1.2.1`，使源码声明与本次发布 tag 对齐。
- 保留并发布 `galay-utils-config-version.cmake` 导出链路，修复下游带版本约束的 `find_package(galay-utils ... CONFIG)` 命中旧 tag 时的版本识别问题。

### Added
- 新增 `scripts/tests/test_cmake_packaging.sh`，回归验证安装后 `find_package(galay-utils <project-version> CONFIG)` 能正确通过版本匹配。
