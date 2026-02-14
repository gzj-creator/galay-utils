import galay.utils;

#include <cassert>
#include <iostream>

int main() {
    using namespace galay::utils;

    auto parts = StringUtils::split("a,b,c", ',');
    assert(parts.size() == 3);
    assert(parts[1] == "b");

    CountingSemaphore sem(1);
    assert(sem.tryAcquire());
    assert(!sem.tryAcquire());
    sem.release();
    assert(sem.available() == 1);

    TokenBucketLimiter limiter(100.0, 10);
    assert(limiter.tryAcquire(1));
    assert(limiter.availableTokens() >= 0.0);

    std::cout << "Module import smoke test passed." << std::endl;
    return 0;
}
