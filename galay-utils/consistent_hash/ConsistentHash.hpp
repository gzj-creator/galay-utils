#ifndef GALAY_UTILS_CONSISTENT_HASH_HPP
#define GALAY_UTILS_CONSISTENT_HASH_HPP

#include "../common/Defn.hpp"
#include <string>
#include <map>
#include <set>
#include <vector>
#include <shared_mutex>
#include <functional>
#include <optional>
#include <atomic>
#include <unordered_map>

namespace galay::utils {

class MurmurHash3 {
public:
    static uint32_t hash32(const void* key, size_t len, uint32_t seed = 0) {
        const uint8_t* data = static_cast<const uint8_t*>(key);
        const int nblocks = static_cast<int>(len / 4);

        uint32_t h1 = seed;
        const uint32_t c1 = 0xcc9e2d51;
        const uint32_t c2 = 0x1b873593;

        const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data + nblocks * 4);

        for (int i = -nblocks; i; i++) {
            uint32_t k1 = blocks[i];

            k1 *= c1;
            k1 = rotl32(k1, 15);
            k1 *= c2;

            h1 ^= k1;
            h1 = rotl32(h1, 13);
            h1 = h1 * 5 + 0xe6546b64;
        }

        const uint8_t* tail = data + nblocks * 4;
        uint32_t k1 = 0;

        switch (len & 3) {
            case 3: k1 ^= tail[2] << 16; [[fallthrough]];
            case 2: k1 ^= tail[1] << 8;  [[fallthrough]];
            case 1: k1 ^= tail[0];
                    k1 *= c1;
                    k1 = rotl32(k1, 15);
                    k1 *= c2;
                    h1 ^= k1;
        }

        h1 ^= static_cast<uint32_t>(len);
        h1 = fmix32(h1);

        return h1;
    }

    static uint32_t hash32(const std::string& key, uint32_t seed = 0) {
        return hash32(key.data(), key.size(), seed);
    }

private:
    static uint32_t rotl32(uint32_t x, int8_t r) {
        return (x << r) | (x >> (32 - r));
    }

    static uint32_t fmix32(uint32_t h) {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;
        return h;
    }
};

struct NodeStatus {
    std::atomic<bool> healthy{true};
    std::atomic<uint64_t> requestCount{0};
    std::atomic<uint64_t> failureCount{0};

    void recordRequest() { ++requestCount; }
    void recordFailure() { ++failureCount; healthy = false; }
    void markHealthy() { healthy = true; }
    void reset() { requestCount = 0; failureCount = 0; healthy = true; }
};

struct NodeConfig {
    std::string id;
    std::string endpoint;
    int weight = 1;

    bool operator==(const NodeConfig& other) const {
        return id == other.id;
    }
};

struct PhysicalNode {
    NodeConfig config;
    NodeStatus status;

    explicit PhysicalNode(NodeConfig cfg) : config(std::move(cfg)) {}
};

class ConsistentHash {
public:
    using HashFunc = std::function<uint32_t(const std::string&)>;

    explicit ConsistentHash(size_t virtualNodes = 150,
                            HashFunc hashFunc = nullptr)
        : m_virtualNodes(virtualNodes)
        , m_hashFunc(hashFunc ? std::move(hashFunc) :
                     [](const std::string& key) { return MurmurHash3::hash32(key); }) {}

    void addNode(const NodeConfig& config) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        auto node = std::make_shared<PhysicalNode>(config);
        m_nodes[config.id] = node;

        size_t vnodes = m_virtualNodes * config.weight;
        for (size_t i = 0; i < vnodes; ++i) {
            std::string virtualKey = config.id + "#" + std::to_string(i);
            uint32_t hash = m_hashFunc(virtualKey);
            m_ring[hash] = node;
        }
    }

    void removeNode(const std::string& nodeId) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_nodes.find(nodeId);
        if (it == m_nodes.end()) {
            return;
        }

        auto& config = it->second->config;
        size_t vnodes = m_virtualNodes * config.weight;

        for (size_t i = 0; i < vnodes; ++i) {
            std::string virtualKey = nodeId + "#" + std::to_string(i);
            uint32_t hash = m_hashFunc(virtualKey);
            m_ring.erase(hash);
        }

        m_nodes.erase(it);
    }

    std::optional<NodeConfig> getNode(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        if (m_ring.empty()) {
            return std::nullopt;
        }

        uint32_t hash = m_hashFunc(key);
        auto it = m_ring.lower_bound(hash);

        if (it == m_ring.end()) {
            it = m_ring.begin();
        }

        it->second->status.recordRequest();
        return it->second->config;
    }

    std::optional<NodeConfig> getHealthyNode(const std::string& key, size_t maxRetries = 3) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        if (m_ring.empty()) {
            return std::nullopt;
        }

        uint32_t hash = m_hashFunc(key);
        auto it = m_ring.lower_bound(hash);

        for (size_t retry = 0; retry < maxRetries; ++retry) {
            if (it == m_ring.end()) {
                it = m_ring.begin();
            }

            if (it->second->status.healthy) {
                it->second->status.recordRequest();
                return it->second->config;
            }

            ++it;
        }

        return std::nullopt;
    }

    std::vector<NodeConfig> getNodes(const std::string& key, size_t count) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        std::vector<NodeConfig> result;
        if (m_ring.empty() || count == 0) {
            return result;
        }

        uint32_t hash = m_hashFunc(key);
        auto it = m_ring.lower_bound(hash);

        std::set<std::string> seen;
        size_t iterations = 0;
        size_t maxIterations = m_ring.size();

        while (result.size() < count && iterations < maxIterations) {
            if (it == m_ring.end()) {
                it = m_ring.begin();
            }

            const auto& nodeId = it->second->config.id;
            if (seen.find(nodeId) == seen.end()) {
                seen.insert(nodeId);
                result.push_back(it->second->config);
            }

            ++it;
            ++iterations;
        }

        return result;
    }

    void markUnhealthy(const std::string& nodeId) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_nodes.find(nodeId);
        if (it != m_nodes.end()) {
            it->second->status.recordFailure();
        }
    }

    void markHealthy(const std::string& nodeId) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_nodes.find(nodeId);
        if (it != m_nodes.end()) {
            it->second->status.markHealthy();
        }
    }

    std::vector<NodeConfig> getAllNodes() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        std::vector<NodeConfig> result;
        result.reserve(m_nodes.size());
        for (const auto& [id, node] : m_nodes) {
            result.push_back(node->config);
        }
        return result;
    }

    size_t nodeCount() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_nodes.size();
    }

    size_t virtualNodeCount() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_ring.size();
    }

    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_nodes.empty();
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_ring.clear();
        m_nodes.clear();
    }

private:
    size_t m_virtualNodes;
    HashFunc m_hashFunc;
    mutable std::shared_mutex m_mutex;
    std::map<uint32_t, std::shared_ptr<PhysicalNode>> m_ring;
    std::unordered_map<std::string, std::shared_ptr<PhysicalNode>> m_nodes;
};

} // namespace galay::utils

#endif // GALAY_UTILS_CONSISTENT_HASH_HPP
