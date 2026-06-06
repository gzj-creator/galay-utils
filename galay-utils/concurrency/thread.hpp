#ifndef GALAY_UTILS_THREAD_HPP
#define GALAY_UTILS_THREAD_HPP

#include "galay-utils/common/defn.hpp"
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <optional>

namespace galay::utils {

/**
 * @file thread.hpp
 * @brief 线程池和线程安全容器
 * @author galay-utils
 * @version 1.0.0
 *
 * @details 提供高性能线程池（ThreadPool）、任务等待器（TaskWaiter）、
 *          线程安全双向链表（ThreadSafeList）等并发工具。
 */

/**
 * @brief 高性能线程池
 * @details 支持任务提交、带返回值的异步任务、优雅停止和任务等待。
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = 0) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) {
                numThreads = 4;
            }
        }

        m_workers.reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i) {
            m_workers.emplace_back([this] { workerLoop(); });
        }
    }

    ~ThreadPool() {
        stop();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief 提交带返回值的异步任务
     * @tparam F 函数类型
     * @tparam Args 参数类型
     * @param f 待执行的函数
     * @param args 函数参数
     * @return 包含返回值的 future 对象
     */
    template<typename F, typename... Args>
    auto addTask(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopped) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            m_tasks.emplace([task]() { (*task)(); });
        }

        m_cv.notify_one();
        return result;
    }

    /**
     * @brief 执行无返回值的异步任务
     * @tparam F 函数类型
     * @param f 待执行的函数
     */
    template<typename F>
    void execute(F&& f) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopped) {
                return;
            }
            m_tasks.emplace(std::forward<F>(f));
        }
        m_cv.notify_one();
    }

    size_t threadCount() const { return m_workers.size(); } ///< 获取线程数量

    /**
     * @brief 获取待处理任务数量
     * @return 待处理任务数量
     */
    size_t pendingTasks() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tasks.size();
    }

    bool isStopped() const { return m_stopped; } ///< 判断线程池是否已停止

    /**
     * @brief 阻塞等待所有任务完成
     * @warning 会阻塞当前线程，不要在协程中使用
     */
    void waitAll() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_completionCv.wait(lock, [this] {
            return m_tasks.empty() && m_activeTasks == 0;
        });
    }

    /**
     * @brief 优雅停止线程池（等待所有任务完成后停止）
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopped) {
                return;
            }
            m_stopped = true;
        }

        m_cv.notify_all();

        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    /**
     * @brief 立即停止线程池（丢弃所有未完成任务）
     */
    void stopNow() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopped = true;
            std::queue<std::function<void()>> empty;
            std::swap(m_tasks, empty);
        }

        m_cv.notify_all();

        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    void workerLoop() {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] {
                    return m_stopped || !m_tasks.empty();
                });

                if (m_stopped && m_tasks.empty()) {
                    return;
                }

                if (!m_tasks.empty()) {
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                    ++m_activeTasks;
                }
            }

            if (task) {
                task();
                --m_activeTasks;
                m_completionCv.notify_all();
            }
        }
    }

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::condition_variable m_completionCv;
    std::atomic<bool> m_stopped{false};
    std::atomic<size_t> m_activeTasks{0};
};

/**
 * @brief 任务等待器
 * @details 用于等待线程池中的多个任务完成。
 */
class TaskWaiter {
public:
    TaskWaiter() : m_count(0) {}

    /**
     * @brief 向线程池添加任务并跟踪计数
     * @tparam F 函数类型
     * @param pool 线程池引用
     * @param f 待执行的任务
     */
    template<typename F>
    void addTask(ThreadPool& pool, F&& f) {
        ++m_count;
        pool.execute([this, func = std::forward<F>(f)]() {
            try {
                func();
            } catch (...) {
            }
            if (--m_count == 0) {
                m_cv.notify_all();
            }
        });
    }

    /**
     * @brief 阻塞等待所有任务完成
     * @warning 会阻塞当前线程，不要在协程中使用
     */
    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return m_count == 0; });
    }

    /**
     * @brief 带超时的阻塞等待
     * @warning 会阻塞当前线程，不要在协程中使用
     */
    template<typename Rep, typename Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, timeout, [this] { return m_count == 0; });
    }

private:
    std::atomic<size_t> m_count;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

/**
 * @brief 线程安全双向链表节点
 * @tparam T 数据类型
 */
template<typename T>
struct ListNode {
    T data;
    ListNode<T>* prev = nullptr;
    ListNode<T>* next = nullptr;

    explicit ListNode(T value) : data(std::move(value)) {}
};

/**
 * @brief 线程安全双向链表
 * @details 使用互斥锁保护所有操作，支持头尾插入、删除和清空。
 * @tparam T 数据类型
 */
template<typename T>
class ThreadSafeList {
public:
    using Node = ListNode<T>;

    ThreadSafeList() = default;

    ~ThreadSafeList() {
        clear();
    }

    ThreadSafeList(const ThreadSafeList&) = delete;
    ThreadSafeList& operator=(const ThreadSafeList&) = delete;

    /**
     * @brief 在链表头部插入元素
     * @param value 插入的值
     * @return 新节点指针
     */
    Node* pushFront(T value) {
        auto node = new Node(std::move(value));
        std::lock_guard<std::mutex> lock(m_mutex);

        node->next = m_head;
        if (m_head) {
            m_head->prev = node;
        }
        m_head = node;
        if (!m_tail) {
            m_tail = node;
        }
        ++m_size;

        return node;
    }

    /**
     * @brief 在链表尾部插入元素
     * @param value 插入的值
     * @return 新节点指针
     */
    Node* pushBack(T value) {
        auto node = new Node(std::move(value));
        std::lock_guard<std::mutex> lock(m_mutex);

        node->prev = m_tail;
        if (m_tail) {
            m_tail->next = node;
        }
        m_tail = node;
        if (!m_head) {
            m_head = node;
        }
        ++m_size;

        return node;
    }

    /**
     * @brief 弹出链表头部元素
     * @return 弹出的值，链表为空时返回 std::nullopt
     */
    std::optional<T> popFront() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_head) {
            return std::nullopt;
        }

        Node* node = m_head;
        T value = std::move(node->data);
        m_head = m_head->next;
        if (m_head) {
            m_head->prev = nullptr;
        } else {
            m_tail = nullptr;
        }
        --m_size;
        delete node;

        return value;
    }

    /**
     * @brief 弹出链表尾部元素
     * @return 弹出的值，链表为空时返回 std::nullopt
     */
    std::optional<T> popBack() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_tail) {
            return std::nullopt;
        }

        Node* node = m_tail;
        T value = std::move(node->data);
        m_tail = m_tail->prev;
        if (m_tail) {
            m_tail->next = nullptr;
        } else {
            m_head = nullptr;
        }
        --m_size;
        delete node;

        return value;
    }

    /**
     * @brief 移除指定节点
     * @param node 目标节点指针
     */
    void remove(Node* node) {
        if (!node) return;

        std::lock_guard<std::mutex> lock(m_mutex);

        if (node->prev) {
            node->prev->next = node->next;
        } else {
            m_head = node->next;
        }

        if (node->next) {
            node->next->prev = node->prev;
        } else {
            m_tail = node->prev;
        }

        --m_size;
        delete node;
    }

    /**
     * @brief 获取链表大小
     * @return 元素数量
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size;
    }

    /**
     * @brief 判断链表是否为空
     * @return 为空返回 true
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size == 0;
    }

    /**
     * @brief 清空链表
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (m_head) {
            Node* next = m_head->next;
            delete m_head;
            m_head = next;
        }
        m_tail = nullptr;
        m_size = 0;
    }

private:
    mutable std::mutex m_mutex;
    Node* m_head = nullptr;
    Node* m_tail = nullptr;
    size_t m_size = 0;
};

} // namespace galay::utils

#endif // GALAY_UTILS_THREAD_HPP
