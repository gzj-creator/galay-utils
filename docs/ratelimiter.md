# RateLimiter 模块

## 概述

RateLimiter 模块提供了多种速率限制算法，用于控制请求频率。

## 主要功能

### 计数信号量

```cpp
CountingSemaphore sem(3); // 允许3个并发

sem.acquire(2); // 获取2个许可
// 执行操作
sem.release(2); // 释放许可
```

### 令牌桶

```cpp
TokenBucketLimiter bucket(100, 10); // 100 tokens/sec, 容量10

if (bucket.tryAcquire(5)) {
    // 处理请求
}
```

### 滑动窗口

```cpp
SlidingWindowLimiter window(5, std::chrono::milliseconds(100)); // 100ms内最多5个请求

if (window.tryAcquire()) {
    // 处理请求
}
```
