#ifndef AFINA_STORAGE_THREAD_SAFE_SIMPLE_LRU_H
#define AFINA_STORAGE_THREAD_SAFE_SIMPLE_LRU_H

#include <map>
#include <mutex>
#include <condition_variable>
#include <string>

#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

/**
 * # SimpleLRU thread safe version
 *
 *
 */
class ThreadSafeSimplLRU : public Storage {
public:
    ThreadSafeSimplLRU(size_t max_size = 1024) : Ex_Storage(max_size) {}
    ~ThreadSafeSimplLRU() {}

    // see SimpleLRU.h
    bool Put(const std::string &key, const std::string &value) override {
        // TODO: sinchronization
        std::lock_guard<std::recursive_mutex> lock(worker_mutex);
        return Ex_Storage.Put(key, value);
    }

    // see SimpleLRU.h
    bool PutIfAbsent(const std::string &key, const std::string &value) override {
        // TODO: sinchronization
        std::lock_guard<std::mutex> lock(worker_mutex);
        return Ex_Storage.PutIfAbsent(key, value);
    }

    // see SimpleLRU.h
    bool Set(const std::string &key, const std::string &value) override {
        // TODO: sinchronization
        std::lock_guard<std::recursive_mutex> lock(worker_mutex);
        return Ex_Storage.Set(key, value);
    }

    // see SimpleLRU.h
    bool Delete(const std::string &key) override {
        // TODO: sinchronization
        std::lock_guard<std::mutex> lock(worker_mutex);
        return Ex_Storage.Delete(key);
    }

    // see SimpleLRU.h
    bool Get(const std::string &key, std::string &value) override {
        // TODO: sinchronization
        std::lock_guard<std::mutex> lock(worker_mutex);
        return Ex_Storage.Get(key, value);
    }

private:
    // TODO: sinchronization primitives
    std::mutex worker_mutex;
    SimpleLRU Ex_Storage;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_THREAD_SAFE_SIMPLE_LRU_H
