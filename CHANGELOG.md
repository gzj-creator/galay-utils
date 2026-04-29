# CHANGELOG

维护说明：
- 未打 tag 的改动先写入 `## [Unreleased]`。
- 需要发版时，把 `Unreleased` 条目整理进新版本节，并补回空的 `Unreleased`。
- 版本格式统一为 `vX.Y.Z`，日期使用 `YYYY-MM-DD`。

## [Unreleased]

### Added
- 新增 `IniParser` 与 `TomlParser`，并补充 `.toml` 扩展名解析注册能力。
- 新增 parser 公共拆分头：`parser_base.hpp`、`detail.hpp`、`config.hpp`、`env.hpp`、`ini.hpp`、`toml.hpp`、`parser_manager.hpp`。

### Changed
- 将原 `parser.hpp` 调整为统一入口封装头，改为包含 `parser_manager.hpp`。
- 更新聚合导出头与 C++ 模块入口中的 parser 引用路径，统一使用 `parser_manager.hpp`。
- 同步更新 README 与 API/使用指南/示例/FAQ 文档中的 parser 类型与默认扩展名说明。

### Fixed
- 补充 parser 相关测试，覆盖 INI/TOML 正常路径与常见非法输入场景。

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
