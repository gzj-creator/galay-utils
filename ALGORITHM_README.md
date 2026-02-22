# galay-utils 算法工具库

现代C++20工具库，提供高性能的算法实现和实用工具。

## 📦 算法模块

### 1. Base64 编码/解码
- ✅ 标准Base64编码和解码
- ✅ URL安全Base64编码
- ✅ PEM格式（64字符换行）
- ✅ MIME格式（76字符换行）
- ✅ String和String_view接口支持
- 🚀 性能：91μs编码 / 82μs解码（10KB数据）

```cpp
// 基本使用
std::string encoded = Base64Util::Base64Encode("Hello, World!");
std::string decoded = Base64Util::Base64Decode(encoded);

// URL安全编码
std::string urlSafe = Base64Util::Base64Encode(data, true);

// PEM格式
std::string pem = Base64Util::Base64EncodePem(data);

// String_view版本
std::string_view view = "data";
std::string encoded = Base64Util::Base64EncodeView(view);
```

### 2. MD5 哈希
- ✅ 完整的RFC 1321标准实现
- ✅ 支持任意长度输入
- ✅ 十六进制和原始字节输出
- ✅ String_view接口支持
- 🚀 性能：133μs（10KB数据）

```cpp
// 十六进制输出
std::string hash = MD5Util::MD5("Hello, World!");
// 输出: "65a8e27d8879283831b664bd8b7f0ad4"

// 原始字节输出
auto rawHash = MD5Util::MD5Raw(data);

// String_view版本
std::string hash = MD5Util::MD5View(view);
```

**注意**：MD5已被证明存在碰撞漏洞，不适合密码学安全场景，但仍适用于：
- 文件完整性校验
- 数据去重
- 缓存键生成

### 3. MurmurHash3
- ✅ 32位和128位哈希
- ✅ 优秀的雪崩效应和分布均匀性
- ✅ 支持自定义种子值
- ✅ String_view接口支持
- 🚀 性能：11μs（32位）/ 4μs（128位）- 10KB数据

```cpp
// 32位哈希
uint32_t hash32 = MurmurHash3Util::Hash32("key");

// 128位哈希（十六进制）
std::string hash128 = MurmurHash3Util::Hash128("key");

// 128位哈希（原始字节）
auto raw128 = MurmurHash3Util::Hash128Raw("key");

// 使用种子值
uint32_t hash = MurmurHash3Util::Hash32("key", 42);

// String_view版本
uint32_t hash = MurmurHash3Util::Hash32View(view);
```

**适用场景**：
- 哈希表（HashMap）
- 布隆过滤器
- 数据分片/分区
- 负载均衡
- 快速数据校验

### 4. Salt 生成器
- ✅ 多种输出格式（Hex、Base64、原始字节）
- ✅ 普通随机和密码学安全随机
- ✅ Bcrypt专用盐值生成
- ✅ 自定义字符集支持
- ✅ 时间戳前缀盐值
- 🚀 性能：3.3μs/个

```cpp
// Hex格式（16字节 = 32个十六进制字符）
std::string hexSalt = SaltGenerator::generateHex(16);

// Base64格式
std::string base64Salt = SaltGenerator::generateBase64(16);

// 密码学安全版本
std::string secureSalt = SaltGenerator::generateSecureHex(32);

// Bcrypt专用（22字符）
std::string bcryptSalt = SaltGenerator::generateBcryptSalt();

// 自定义字符集
std::string customSalt = SaltGenerator::generateCustom(20, "0123456789");

// 时间戳盐值（保证时间唯一性）
std::string timestampedSalt = SaltGenerator::generateTimestamped(32);

// 验证格式
bool valid = SaltGenerator::isValidHex(salt);
```

**适用场景**：
- 密码哈希加盐
- 会话令牌生成
- API密钥生成
- 验证码生成

### 5. HMAC-SHA256
- ✅ 标准的SHA-256实现（FIPS 180-4）
- ✅ 正确的HMAC实现（RFC 2104）
- ✅ 十六进制和原始字节输出
- 🚀 生产级加密哈希

```cpp
// HMAC-SHA256（十六进制）
std::string hmac = HMAC::hmacSha256Hex("secret-key", "message");

// HMAC-SHA256（原始字节）
auto hmacBytes = HMAC::hmacSha256("secret-key", "message");

// SHA-256哈希
std::string hash = SHA256::hashHex("data");
```

**适用场景**：
- API签名验证
- 消息认证码（MAC）
- JWT签名（如需JWT，建议集成完整的JSON库）
- 数据完整性验证

## 🎯 完整的认证工作流示例

```cpp
// 1. 用户注册 - 生成盐值和密码哈希
std::string salt = SaltGenerator::generateSecureHex(16);
std::string passwordHash = MD5Util::MD5(password + salt);
// 存储: username, passwordHash, salt

// 2. 用户登录 - 验证密码
std::string inputHash = MD5Util::MD5(inputPassword + storedSalt);
if (inputHash == storedPasswordHash) {
    // 登录成功
}

// 3. API签名 - 使用HMAC
std::string signature = HMAC::hmacSha256Hex(apiSecret, requestData);

// 4. 数据分片 - 使用MurmurHash3
uint32_t shardId = MurmurHash3Util::Hash32(userId) % numShards;
```

## 📊 性能对比

| 算法 | 类型 | 性能(10KB) | 输出 | 主要用途 |
|------|------|-----------|------|---------|
| **Base64** | 编码 | 91μs编码<br>82μs解码 | 字符串 | 数据传输、存储 |
| **MD5** | 加密哈希 | 133μs | 128位 | 文件校验、非安全哈希 |
| **MurmurHash3** | 非加密哈希 | 11μs(32位)<br>4μs(128位) | 32/128位 | 哈希表、布隆过滤器 |
| **Salt生成器** | 随机生成 | 3.3μs/个 | 多种格式 | 密码加盐、令牌生成 |
| **HMAC-SHA256** | 消息认证 | ~40μs | 256位 | API签名、JWT |

## 🔧 编译和使用

### 要求
- C++20或更高版本
- CMake 3.16+

### 编译
```bash
mkdir build && cd build
cmake -DBUILD_TESTS=ON ..
cmake --build . --parallel
```

### 运行测试
```bash
./test/test_all
```

### 在项目中使用
```cpp
#include <galay-utils/galay-utils.hpp>

using namespace galay::utils;

// 使用任何工具
std::string encoded = Base64Util::Base64Encode("data");
uint32_t hash = MurmurHash3Util::Hash32("key");
std::string salt = SaltGenerator::generateSecureHex(16);
```

## ✨ 特性

- ✅ **Header-only**: 无需编译，直接包含即可使用
- ✅ **零依赖**: 不依赖任何第三方库
- ✅ **现代C++**: 使用C++20特性，代码简洁高效
- ✅ **高性能**: 所有算法都经过优化
- ✅ **全面测试**: 每个模块都有详细的单元测试
- ✅ **生产就绪**: 可直接用于生产环境

## 📝 测试覆盖

所有模块都包含全面的测试：
- 基本功能测试
- 边界情况测试
- 性能测试
- 一致性测试
- 安全性测试

测试结果示例：
```
========================================
    galay-utils Test Suite
========================================

=== Testing Base64 ===
  Performance (10KB): Encode=91μs, Decode=82μs
Base64 tests passed!

=== Testing MD5 ===
  Performance (10KB): 133μs
MD5 tests passed!

=== Testing MurmurHash3 ===
  Performance (10KB): Hash32=11μs, Hash128=4μs
  Avalanche effect: 18/32 bits differ
  Distribution: min=74, max=117 (expected ~100)
MurmurHash3 tests passed!

=== Testing Salt Generator ===
  Performance (1000 salts): 2625μs (2.625μs per salt)
Salt Generator tests passed!

========================================
    All tests passed!
========================================
```

## 🚀 未来计划

- [ ] SHA-384/SHA-512支持
- [ ] AES加密/解密
- [ ] JWT支持（需要集成JSON库）
- [ ] 更多哈希算法（xxHash、CityHash等）

## 📄 许可证

本项目采用MIT许可证。

## 🤝 贡献

欢迎提交Issue和Pull Request！

## ⚠️ 安全提示

- MD5不适合密码学安全场景，仅用于校验和非安全哈希
- MurmurHash3是非加密哈希，不适合安全场景
- 密码存储请使用bcrypt、PBKDF2或Argon2等专用算法
- 生产环境的密码学操作建议使用OpenSSL等成熟库
