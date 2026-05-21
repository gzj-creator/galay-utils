/**
 * @file galay_utils.hpp
 * @brief galay-utils 总头文件
 * @author galay-utils
 * @version 1.0.0
 *
 * @details 包含 galay-utils 库的所有模块头文件，提供一站式引用。
 */

#ifndef GALAY_UTILS_HPP
#define GALAY_UTILS_HPP

/// 公共类型定义
#include "galay-utils/common/defn.hpp"
/// 类型名称工具
#include "galay-utils/common/type_name.hpp"

/// 字符串工具
#include "string/string.hpp"

/// 随机数生成
#include "random/random.hpp"

/// 系统工具
#include "galay-utils/system/system.hpp"

/// 堆栈跟踪
#include "backtrace/trace.hpp"

/// 信号处理
#include "signal/signal.hpp"

/// 对象池
#include "pool/pool.hpp"

/// 线程池
#include "thread/thread.hpp"

/// 熔断器
#include "circuitbreaker/breaker.hpp"

/// 一致性哈希
#include "consistent_hash/hash.hpp"

/// 字典树
#include "trie/trie.hpp"

/// 哈夫曼编码
#include "huffman/huffman.hpp"

/// 多版本并发控制
#include "mvcc/mvcc.hpp"

/// 命令行参数解析
#include "args/app.hpp"

/// 配置文件解析
#include "galay-utils/parser/parser_manager.hpp"

/// 进程管理
#include "process/process.hpp"

/// 负载均衡
#include "balancer/balancer.hpp"

/// Base64 编解码
#include "galay-utils/algorithm/base64.hpp"
/// MD5 哈希
#include "galay-utils/algorithm/md5.hpp"
/// MurmurHash3 哈希
#include "galay-utils/algorithm/murmur_hash3.hpp"
/// 盐值生成
#include "galay-utils/algorithm/salt.hpp"
/// HMAC-SHA256
#include "galay-utils/algorithm/hmac.hpp"

#endif // GALAY_UTILS_HPP
