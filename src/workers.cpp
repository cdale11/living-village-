#include "living_village/workers.hpp"
#include <algorithm>
#include <condition_variable>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
namespace village {
namespace {
class Pool {
public:
    explicit Pool(unsigned n) : total(n) {
        try { for (unsigned id = 1; id < n; ++id) threads.emplace_back([this, id] { work(id); }); }
        catch (...) { stop(); throw; }
    }
    ~Pool() { stop(); }
    void run(std::size_t n, const std::function<void(std::size_t, std::size_t)>& function) {
        std::lock_guard<std::mutex> dispatch(serial);
        {
            std::lock_guard<std::mutex> lock(mutex);
            count = n; job = &function; remaining = total - 1; failure = nullptr; ++generation;
        }
        ready.notify_all(); execute(0);
        std::unique_lock<std::mutex> lock(mutex);
        finished.wait(lock, [&] { return remaining == 0; });
        job = nullptr;
        if (failure) std::rethrow_exception(failure);
    }
    unsigned total;
private:
    std::vector<std::thread> threads;
    std::mutex mutex, serial;
    std::condition_variable ready, finished;
    bool stopping = false;
    unsigned remaining = 0;
    std::size_t generation = 0, count = 0;
    const std::function<void(std::size_t, std::size_t)>* job = nullptr;
    std::exception_ptr failure;
    void execute(unsigned id) {
        try { (*job)(count * id / total, count * (id + 1) / total); }
        catch (...) { std::lock_guard<std::mutex> lock(mutex); if (!failure) failure = std::current_exception(); }
    }
    void work(unsigned id) {
        std::size_t seen = 0;
        std::unique_lock<std::mutex> lock(mutex);
        for (;;) {
            ready.wait(lock, [&] { return stopping || generation != seen; });
            if (stopping) return;
            seen = generation; lock.unlock(); execute(id); lock.lock();
            if (--remaining == 0) finished.notify_one();
        }
    }
    void stop() {
        { std::lock_guard<std::mutex> lock(mutex); stopping = true; }
        ready.notify_all();
        for (auto& thread : threads) if (thread.joinable()) thread.join();
    }
};
std::unique_ptr<Pool> pool;
}
void configure_workers(unsigned count) {
    count = count == 0 ? std::min(8u, std::max(1u, std::thread::hardware_concurrency())) : std::clamp(count, 1u, 8u);
    // Configuration occurs between updates, never from worker callbacks.
    if (pool && pool->total == count) return;
    pool.reset();
    if (count > 1) try { pool = std::make_unique<Pool>(count); } catch (...) { pool.reset(); }
}
unsigned worker_count() { return pool ? pool->total : 1; }
void parallel_tiles(std::size_t count, const std::function<void(std::size_t, std::size_t)>& job) {
    if (pool && count >= 1024) pool->run(count, job); else job(0, count);
}
}
