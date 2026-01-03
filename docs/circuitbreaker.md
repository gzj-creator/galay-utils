# CircuitBreaker 模块

## 概述

CircuitBreaker 模块实现了熔断器模式，用于保护服务免受级联故障的影响。

## 主要功能

### 熔断器配置

```cpp
CircuitBreakerConfig config;
config.failureThreshold = 3;        // 失败阈值
config.successThreshold = 2;        // 成功阈值
config.resetTimeout = std::chrono::seconds(1); // 重置超时

CircuitBreaker cb(config);
```

### 熔断器状态

```cpp
CircuitState state = cb.state();
// Closed: 关闭状态，允许请求
// Open: 开放状态，快速失败
// HalfOpen: 半开放状态，测试恢复
```

### 请求处理

```cpp
if (cb.allowRequest()) {
    try {
        // 执行请求
        cb.onSuccess();
    } catch (...) {
        cb.onFailure();
    }
} else {
    // 熔断器开放，快速失败
}
```
