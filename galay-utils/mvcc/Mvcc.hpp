#ifndef GALAY_UTILS_MVCC_HPP
#define GALAY_UTILS_MVCC_HPP

#include "../common/Defn.hpp"
#include <atomic>
#include <shared_mutex>
#include <map>
#include <memory>
#include <optional>
#include <functional>
#include <vector>

namespace galay::utils {

using Version = uint64_t;

template<typename T>
struct VersionedValue {
    Version version;
    std::unique_ptr<T> value;
    bool deleted;

    VersionedValue(Version v, std::unique_ptr<T> val, bool del = false)
        : version(v), value(std::move(val)), deleted(del) {}
};

template<typename T>
class Mvcc {
public:
    Mvcc() : m_currentVersion(0) {}

    const T* getValue(Version version) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_versions.upper_bound(version);
        if (it == m_versions.begin()) {
            return nullptr;
        }
        --it;

        if (it->second->deleted) {
            return nullptr;
        }

        return it->second->value.get();
    }

    const T* getCurrentValue() const {
        return getValue(m_currentVersion.load());
    }

    std::pair<const T*, Version> getValueWithVersion(Version version) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_versions.upper_bound(version);
        if (it == m_versions.begin()) {
            return {nullptr, 0};
        }
        --it;

        if (it->second->deleted) {
            return {nullptr, it->first};
        }

        return {it->second->value.get(), it->first};
    }

    Version putValue(std::unique_ptr<T> value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        Version newVersion = ++m_currentVersion;
        m_versions[newVersion] = std::make_unique<VersionedValue<T>>(
            newVersion, std::move(value), false);

        return newVersion;
    }

    Version putValue(const T& value) {
        return putValue(std::make_unique<T>(value));
    }

    Version updateValue(std::function<std::unique_ptr<T>(const T*)> updateFn) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        const T* current = nullptr;
        if (!m_versions.empty()) {
            auto it = m_versions.rbegin();
            if (!it->second->deleted) {
                current = it->second->value.get();
            }
        }

        auto newValue = updateFn(current);
        Version newVersion = ++m_currentVersion;
        m_versions[newVersion] = std::make_unique<VersionedValue<T>>(
            newVersion, std::move(newValue), false);

        return newVersion;
    }

    Version compareAndSwap(Version expectedVersion, std::unique_ptr<T> newValue) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        if (m_currentVersion.load() != expectedVersion) {
            return 0;
        }

        Version newVersion = ++m_currentVersion;
        m_versions[newVersion] = std::make_unique<VersionedValue<T>>(
            newVersion, std::move(newValue), false);

        return newVersion;
    }

    bool removeValue(Version version) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_versions.find(version);
        if (it == m_versions.end()) {
            return false;
        }

        m_versions.erase(it);
        return true;
    }

    Version deleteValue() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        Version newVersion = ++m_currentVersion;
        m_versions[newVersion] = std::make_unique<VersionedValue<T>>(
            newVersion, nullptr, true);

        return newVersion;
    }

    bool isValid(Version version) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_versions.find(version) != m_versions.end();
    }

    Version currentVersion() const {
        return m_currentVersion.load();
    }

    size_t versionCount() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_versions.size();
    }

    void gc(size_t keepVersions) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        while (m_versions.size() > keepVersions) {
            m_versions.erase(m_versions.begin());
        }
    }

    void gcOlderThan(Version olderThan) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_versions.begin();
        while (it != m_versions.end() && it->first < olderThan) {
            it = m_versions.erase(it);
        }
    }

    std::vector<Version> getAllVersions() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        std::vector<Version> result;
        result.reserve(m_versions.size());
        for (const auto& [ver, _] : m_versions) {
            result.push_back(ver);
        }
        return result;
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_versions.clear();
        m_currentVersion = 0;
    }

private:
    mutable std::shared_mutex m_mutex;
    std::atomic<Version> m_currentVersion;
    std::map<Version, std::unique_ptr<VersionedValue<T>>> m_versions;
};

class Snapshot {
public:
    explicit Snapshot(Version version) : m_version(version) {}

    Version version() const { return m_version; }

    template<typename T>
    const T* read(const Mvcc<T>& mvcc) const {
        return mvcc.getValue(m_version);
    }

private:
    Version m_version;
};

template<typename T>
class Transaction {
public:
    explicit Transaction(Mvcc<T>& mvcc)
        : m_mvcc(mvcc)
        , m_startVersion(mvcc.currentVersion())
        , m_committed(false) {}

    const T* read() const {
        return m_mvcc.getValue(m_startVersion);
    }

    void write(std::unique_ptr<T> value) {
        m_pendingValue = std::move(value);
    }

    bool commit() {
        if (m_committed || !m_pendingValue) {
            return false;
        }

        Version newVersion = m_mvcc.compareAndSwap(m_startVersion, std::move(m_pendingValue));
        m_committed = (newVersion != 0);
        return m_committed;
    }

    bool isCommitted() const { return m_committed; }

private:
    Mvcc<T>& m_mvcc;
    Version m_startVersion;
    std::unique_ptr<T> m_pendingValue;
    bool m_committed;
};

} // namespace galay::utils

#endif // GALAY_UTILS_MVCC_HPP
