#ifndef GALAY_UTILS_POOL_HPP
#define GALAY_UTILS_POOL_HPP

#include "../common/Defn.hpp"
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace galay::utils {

/**
 * @brief Base class for poolable objects
 */
class PoolableObject {
public:
    virtual ~PoolableObject() = default;
    virtual void reset() {}
};

/**
 * @brief Concept for poolable objects
 */
template<typename T>
concept IsPoolable = std::is_base_of_v<PoolableObject, T> || requires(T t) {
    { t.reset() } -> std::same_as<void>;
};

/**
 * @brief Generic object pool with RAII support
 */
template<typename T>
class ObjectPool {
public:
    using Ptr = std::unique_ptr<T, std::function<void(T*)>>;
    using Creator = std::function<T*()>;
    using Destroyer = std::function<void(T*)>;

    explicit ObjectPool(size_t initialSize = 0,
                        size_t maxSize = 0,
                        Creator creator = nullptr,
                        Destroyer destroyer = nullptr)
        : m_maxSize(maxSize)
        , m_creator(creator ? std::move(creator) : []() { return new T(); })
        , m_destroyer(destroyer ? std::move(destroyer) : [](T* p) { delete p; })
        , m_totalCreated(0)
        , m_poolSize(0) {
        for (size_t i = 0; i < initialSize; ++i) {
            m_pool.push(m_creator());
            ++m_totalCreated;
            ++m_poolSize;
        }
    }

    ~ObjectPool() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_pool.empty()) {
            m_destroyer(m_pool.front());
            m_pool.pop();
        }
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    Ptr acquire() {
        T* obj = nullptr;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_pool.empty()) {
                obj = m_pool.front();
                m_pool.pop();
                --m_poolSize;
            }
        }

        if (!obj) {
            obj = m_creator();
            ++m_totalCreated;
        }

        return Ptr(obj, [this](T* p) {
            if (p) {
                if constexpr (IsPoolable<T>) {
                    p->reset();
                }
                release(p);
            }
        });
    }

    Ptr tryAcquire() {
        T* obj = nullptr;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_pool.empty()) {
                obj = m_pool.front();
                m_pool.pop();
                --m_poolSize;
            }
        }

        if (!obj) {
            return nullptr;
        }

        return Ptr(obj, [this](T* p) {
            if (p) {
                if constexpr (IsPoolable<T>) {
                    p->reset();
                }
                release(p);
            }
        });
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.size();
    }

    size_t totalCreated() const {
        return m_totalCreated.load();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_pool.empty()) {
            m_destroyer(m_pool.front());
            m_pool.pop();
        }
        m_poolSize = 0;
    }

    void shrink(size_t targetSize) {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (m_pool.size() > targetSize) {
            m_destroyer(m_pool.front());
            m_pool.pop();
            --m_poolSize;
        }
    }

private:
    void release(T* obj) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_maxSize == 0 || m_pool.size() < m_maxSize) {
            m_pool.push(obj);
            ++m_poolSize;
        } else {
            m_destroyer(obj);
        }
    }

    mutable std::mutex m_mutex;
    std::queue<T*> m_pool;
    size_t m_maxSize;
    Creator m_creator;
    Destroyer m_destroyer;
    std::atomic<size_t> m_totalCreated;
    std::atomic<size_t> m_poolSize;
};

/**
 * @brief Thread-safe blocking object pool
 */
template<typename T>
class BlockingObjectPool {
public:
    using Ptr = std::unique_ptr<T, std::function<void(T*)>>;
    using Creator = std::function<T*()>;
    using Destroyer = std::function<void(T*)>;

    explicit BlockingObjectPool(size_t poolSize,
                                Creator creator = nullptr,
                                Destroyer destroyer = nullptr)
        : m_creator(creator ? std::move(creator) : []() { return new T(); })
        , m_destroyer(destroyer ? std::move(destroyer) : [](T* p) { delete p; }) {
        for (size_t i = 0; i < poolSize; ++i) {
            m_pool.push(m_creator());
        }
    }

    ~BlockingObjectPool() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_pool.empty()) {
            m_destroyer(m_pool.front());
            m_pool.pop();
        }
    }

    BlockingObjectPool(const BlockingObjectPool&) = delete;
    BlockingObjectPool& operator=(const BlockingObjectPool&) = delete;

    /**
     * @brief 阻塞获取对象
     * @warning 会阻塞当前线程，不要在协程中使用
     */
    Ptr acquire() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_pool.empty(); });

        T* obj = m_pool.front();
        m_pool.pop();

        return Ptr(obj, [this](T* p) {
            if (p) {
                if constexpr (IsPoolable<T>) {
                    p->reset();
                }
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_pool.push(p);
                }
                m_cv.notify_one();
            }
        });
    }

    /**
     * @brief 带超时的阻塞获取
     * @warning 会阻塞当前线程，不要在协程中使用
     */
    template<typename Rep, typename Period>
    Ptr tryAcquireFor(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, timeout, [this] { return !m_pool.empty(); })) {
            return nullptr;
        }

        T* obj = m_pool.front();
        m_pool.pop();

        return Ptr(obj, [this](T* p) {
            if (p) {
                if constexpr (IsPoolable<T>) {
                    p->reset();
                }
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_pool.push(p);
                }
                m_cv.notify_one();
            }
        });
    }

    size_t available() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pool.size();
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<T*> m_pool;
    Creator m_creator;
    Destroyer m_destroyer;
};

} // namespace galay::utils

#endif // GALAY_UTILS_POOL_HPP
