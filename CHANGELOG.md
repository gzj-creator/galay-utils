# CHANGELOG

维护说明：
- 未打 tag 的改动先写入 `## [Unreleased]`。
- 需要发版时，从 `Unreleased` 或“自上次 tag 以来”的累计变更整理出新的版本节。
- 版本号遵循 `major/minor/patch` 规则：大改动升主版本，新功能升次版本，修复与非破坏性维护升修订版本。
- 推荐标题格式为 `## [vX.Y.Z] - YYYY-MM-DD`，正文按 `Added` / `Changed` / `Fixed` / `Docs` / `Chore` 归纳。

## [Unreleased]

## [v1.2.1] - 2026-04-23

### Fixed
- 将 `CMakeLists.txt` 中的项目版本提升到 `1.2.1`，使源码声明与本次发布 tag 对齐。
- 保留并发布 `galay-utils-config-version.cmake` 导出链路，修复下游带版本约束的 `find_package(galay-utils ... CONFIG)` 命中旧 tag 时的版本识别问题。

### Added
- 新增 `scripts/tests/test_cmake_packaging.sh`，回归验证安装后 `find_package(galay-utils <project-version> CONFIG)` 能正确通过版本匹配。
