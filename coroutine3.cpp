#include <iostream>
#include <coroutine>
#include <thread>
#include <vector>
#include <atomic>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

constexpr int FIB_N = 25;
constexpr int TOTAL_TASKS = 1000;

struct RdtscClock {
    static constexpr double CPU_FREQUENCY_GHZ = 2.8;

    static uint64_t now() {
        _mm_mfence();
        return __rdtsc();
    }

    static double to_ns(uint64_t ticks) {
        return static_cast<double>(ticks) / CPU_FREQUENCY_GHZ;
    }
};

double fib(double n) {
    if (n <= 1.0) return n;
    return fib(n - 1.0) + fib(n - 2.0);
}

// Method 1: Sequential
void test_sequential(int n, int count) {
    std::cout << "\n=== Sequential (No Context Switch) ===\n";
    
    auto t_start = RdtscClock::now();
    
    for (int i = 0; i < count; ++i) {
        volatile double result = fib(static_cast<double>(n));
    }
    
    auto t_end = RdtscClock::now();
    uint64_t total_time = t_end - t_start;
    
    std::cout << "  Total time:       " << total_time << " ticks (" 
              << RdtscClock::to_ns(total_time) << " ns)\n";
    std::cout << "  Avg per task:     " << (total_time / count) << " ticks (" 
              << RdtscClock::to_ns(total_time / count) << " ns)\n";
    std::cout << "  Throughput:       " << (count * 1e9 / RdtscClock::to_ns(total_time)) 
              << " tasks/sec\n";
}

// Method 2: Thread-based (one thread per task)
void test_threads(int n, int count) {
    std::cout << "\n=== Thread-based (" << count << " threads, one per task) ===\n";
    
    auto t_start = RdtscClock::now();
    
    std::vector<std::thread> threads;
    threads.reserve(count);
    
    std::atomic<bool> start_flag{false};
    
    auto t_create_start = RdtscClock::now();
    
    // Create threads that wait for start signal
    for (int i = 0; i < count; ++i) {
        threads.emplace_back([n, &start_flag]() {
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            volatile double result = fib(static_cast<double>(n));
        });
    }
    
    auto t_create_end = RdtscClock::now();
    uint64_t creation_time = t_create_end - t_create_start;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Start all threads simultaneously
    auto t_compute_start = RdtscClock::now();
    start_flag.store(true, std::memory_order_release);
    
    for (auto& thread : threads) {
        thread.join();
    }
    auto t_compute_end = RdtscClock::now();
    uint64_t compute_and_join_time = t_compute_end - t_compute_start;
    
    auto t_end = RdtscClock::now();
    uint64_t total_time = t_end - t_start;
    
    std::cout << "  Total time:       " << total_time << " ticks (" 
              << RdtscClock::to_ns(total_time) << " ns)\n";
    std::cout << "  Creation time:    " << creation_time << " ticks (" 
              << RdtscClock::to_ns(creation_time) << " ns, "
              << (creation_time * 100.0 / total_time) << "%)\n";
    std::cout << "  Compute time:      " << compute_and_join_time << " ticks (" 
              << RdtscClock::to_ns(compute_and_join_time) << " ns, "
              << (compute_and_join_time * 100.0 / total_time) << "%) - Parallel execution\n";
    std::cout << "  Avg per task:     " << (total_time / count) << " ticks (" 
              << RdtscClock::to_ns(total_time / count) << " ns)\n";
    std::cout << "  Throughput:       " << (count * 1e9 / RdtscClock::to_ns(total_time)) 
              << " tasks/sec\n";
    std::cout << "  Avg creation:     " << (creation_time / count) << " ticks ("
              << RdtscClock::to_ns(creation_time / count) << " ns)\n";
}

// Method 3: Coroutine-based (co_return)
thread_local uint64_t g_coro_creation_overhead = 0;
thread_local uint64_t g_coro_destruction_overhead = 0;
thread_local uint64_t g_coro_count = 0;
thread_local uint64_t g_last_pure_compute_time = 0;

struct Task {
    struct promise_type {
        double value = 0.0;
        uint64_t creation_start = 0;
        
        promise_type() {
            creation_start = RdtscClock::now();
        }
        
        Task get_return_object() {
            auto t_end = RdtscClock::now();
            g_coro_creation_overhead += (t_end - creation_start);
            g_coro_count++;
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }  // Suspend at start
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(double v) { value = v; }
        void unhandled_exception() { std::terminate(); }
    };
    
    std::coroutine_handle<promise_type> handle;
    
    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    
    Task(Task&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    
    ~Task() {
        if (handle) {
            auto t_start = RdtscClock::now();
            handle.destroy();
            auto t_end = RdtscClock::now();
            g_coro_destruction_overhead += (t_end - t_start);
        }
    }
    
    void resume() {
        if (handle && !handle.done()) {
            handle.resume();
        }
    }
    
    double result() const { return handle.promise().value; }
};

Task fib_task(double n) {
    auto compute_start = RdtscClock::now();
    double result = fib(n);
    auto compute_end = RdtscClock::now();
    
    g_last_pure_compute_time = compute_end - compute_start;
    
    co_return result;
}

void test_coroutines(int n, int count) {
    std::cout << "\n=== Coroutine-based ===\n";
    
    g_coro_creation_overhead = 0;
    g_coro_destruction_overhead = 0;
    g_coro_count = 0;
    
    auto t_total_start = RdtscClock::now();
    
    uint64_t total_resume_time = 0;  // Total time for resume() calls
    uint64_t total_pure_compute_time = 0;  // Pure compute time inside coroutine
    uint64_t total_switch_overhead = 0;  // Switch overhead = total_resume_time - total_pure_compute_time
    
    for (int i = 0; i < count; ++i) {
        auto task = fib_task(static_cast<double>(n));
        
        // Measure total resume time (includes switch overhead + compute time)
        auto t_resume_start = RdtscClock::now();
        task.resume();
        auto t_resume_end = RdtscClock::now();
        total_resume_time += (t_resume_end - t_resume_start);
        
        total_pure_compute_time += g_last_pure_compute_time;
        
        volatile double result = task.result();
    }
    
    total_switch_overhead = total_resume_time - total_pure_compute_time;
    
    auto t_total_end = RdtscClock::now();
    uint64_t total_time = t_total_end - t_total_start;
    
    std::cout << "  Total time:       " << total_time << " ticks (" 
              << RdtscClock::to_ns(total_time) << " ns)\n";
    std::cout << "  Creation overhead: " << g_coro_creation_overhead << " ticks (" 
              << RdtscClock::to_ns(g_coro_creation_overhead) << " ns, "
              << (g_coro_creation_overhead * 100.0 / total_time) << "%)\n";
    std::cout << "  Destruction overhead: " << g_coro_destruction_overhead << " ticks (" 
              << RdtscClock::to_ns(g_coro_destruction_overhead) << " ns, "
              << (g_coro_destruction_overhead * 100.0 / total_time) << "%)\n";
    
    std::cout << "  Resume time:      " << total_resume_time << " ticks (" 
              << RdtscClock::to_ns(total_resume_time) << " ns, "
              << (total_resume_time * 100.0 / total_time) << "%) - Includes switch + compute\n";
    std::cout << "  Compute time:      " << total_pure_compute_time << " ticks (" 
              << RdtscClock::to_ns(total_pure_compute_time) << " ns, "
              << (total_pure_compute_time * 100.0 / total_time) << "%) - Measured inside coroutine\n";
    std::cout << "  Switch overhead:  " << total_switch_overhead << " ticks (" 
              << RdtscClock::to_ns(total_switch_overhead) << " ns, "
              << (total_switch_overhead * 100.0 / total_time) << "%) - Resume time - Compute time\n";
    
    std::cout << "  Avg per task:     " << (total_time / count) << " ticks (" 
              << RdtscClock::to_ns(total_time / count) << " ns)\n";
    std::cout << "  Throughput:       " << (count * 1e9 / RdtscClock::to_ns(total_time)) 
              << " tasks/sec\n";
    std::cout << "  Avg creation:     " << (g_coro_creation_overhead / count) << " ticks ("
              << RdtscClock::to_ns(g_coro_creation_overhead / count) << " ns)\n";
    std::cout << "  Avg destruction:  " << (g_coro_destruction_overhead / count) << " ticks ("
              << RdtscClock::to_ns(g_coro_destruction_overhead / count) << " ns)\n";
    std::cout << "  Avg switch overhead: " << (total_switch_overhead / count) << " ticks ("
              << RdtscClock::to_ns(total_switch_overhead / count) << " ns)\n";
}

// Method 4: Coroutine-based with co_yield
thread_local uint64_t g_resume_time = 0;
thread_local uint64_t g_yield_count = 0;
thread_local uint64_t g_resume_count = 0;
thread_local uint64_t g_gen_creation_overhead = 0;
thread_local uint64_t g_gen_destruction_overhead = 0;
thread_local uint64_t g_pure_compute_time = 0;

struct Generator {
    struct promise_type {
        double current_value = 0.0;
        uint64_t creation_start = 0;
        
        promise_type() {
            creation_start = RdtscClock::now();
        }
        
        Generator get_return_object() {
            auto t_end = RdtscClock::now();
            g_gen_creation_overhead += (t_end - creation_start);
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        std::suspend_always yield_value(double value) noexcept {
            current_value = value;
            g_yield_count++;
            return {};
        }
        
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    
    std::coroutine_handle<promise_type> handle;
    
    explicit Generator(std::coroutine_handle<promise_type> h) : handle(h) {}
    
    Generator(Generator&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    
    ~Generator() {
        if (handle) {
            auto t_start = RdtscClock::now();
            handle.destroy();
            auto t_end = RdtscClock::now();
            g_gen_destruction_overhead += (t_end - t_start);
        }
    }
    
    bool resume() {
        if (handle && !handle.done()) {
            auto t_start = RdtscClock::now();
            handle.resume();
            auto t_end = RdtscClock::now();
            g_resume_time += (t_end - t_start);
            g_resume_count++;
            return !handle.done();
        }
        return false;
    }
    
    double value() const { 
        return handle.promise().current_value; 
    }
    
    bool done() const {
        return handle.done();
    }
};

Generator fib_generator(double n, int count) {
    for (int i = 0; i < count; ++i) {
        auto t_compute_start = RdtscClock::now();
        double result = fib(n);
        auto t_compute_end = RdtscClock::now();
        g_pure_compute_time += (t_compute_end - t_compute_start);
        co_yield result;
    }
}

void test_coroutines_yield(int n, int count) {
    std::cout << "\n=== Coroutine-based with co_yield ===\n";
    
    g_resume_time = 0;
    g_pure_compute_time = 0;
    g_yield_count = 0;
    g_resume_count = 0;
    g_gen_creation_overhead = 0;
    g_gen_destruction_overhead = 0;
    
    auto t_total_start = RdtscClock::now();
    
    uint64_t compute_time = 0;
    
    auto gen = fib_generator(static_cast<double>(n), count);
    
    for (int i = 0; i < count; ++i) {
        gen.resume();
        volatile double result = gen.value();
    }
    
    auto t_total_end = RdtscClock::now();
    uint64_t total_time = t_total_end - t_total_start;
    
    uint64_t total_switch_overhead = g_resume_time - g_pure_compute_time;
    
    std::cout << "  Total time:       " << total_time << " ticks (" 
        << RdtscClock::to_ns(total_time) << " ns)\n";
    std::cout << "  Creation overhead: " << g_gen_creation_overhead << " ticks (" 
        << RdtscClock::to_ns(g_gen_creation_overhead) << " ns, "
        << (g_gen_creation_overhead * 100.0 / total_time) << "%)\n";
    std::cout << "  Destruction overhead: " << g_gen_destruction_overhead << " ticks (" 
        << RdtscClock::to_ns(g_gen_destruction_overhead) << " ns, "
        << (g_gen_destruction_overhead * 100.0 / total_time) << "%)\n";

    std::cout << "  Resume time:      " << g_resume_time << " ticks (" 
        << RdtscClock::to_ns(g_resume_time) << " ns, "
        << (g_resume_time * 100.0 / total_time) << "%) - Includes switch + compute\n";
    std::cout << "  Compute time:      " << g_pure_compute_time << " ticks (" 
        << RdtscClock::to_ns(g_pure_compute_time) << " ns, "
        << (g_pure_compute_time * 100.0 / total_time) << "%) - Measured inside coroutine\n";
    std::cout << "  Switch overhead:  " << total_switch_overhead << " ticks (" 
        << RdtscClock::to_ns(total_switch_overhead) << " ns, "
        << (total_switch_overhead * 100.0 / total_time) << "%) - Resume time - Compute time\n";

    std::cout << "  Avg per task:     " << (total_time / count) << " ticks (" 
        << RdtscClock::to_ns(total_time / count) << " ns)\n";
    std::cout << "  Throughput:       " << (count * 1e9 / RdtscClock::to_ns(total_time)) 
        << " tasks/sec\n";
    std::cout << "  Yield/Resume count:  " << g_resume_count << "\n";

    std::cout << "  Avg resume time:  " << (g_resume_time / g_resume_count) << " ticks ("
        << RdtscClock::to_ns(g_resume_time / g_resume_count) << " ns)\n";
    std::cout << "  Avg pure compute: " << (g_pure_compute_time / g_resume_count) << " ticks ("
        << RdtscClock::to_ns(g_pure_compute_time / g_resume_count) << " ns)\n";
    std::cout << "  Avg switch overhead: " << (total_switch_overhead / g_resume_count) << " ticks ("
        << RdtscClock::to_ns(total_switch_overhead / g_resume_count) << " ns)\n";
}

int main() {
    std::cout << "Context Switch Overhead Comparison\n";
    std::cout << "Configuration:\n";
    std::cout << "  Fibonacci N:      " << FIB_N << "\n";
    std::cout << "  Total tasks:      " << TOTAL_TASKS << "\n";
    std::cout << "  CPU frequency:    " << RdtscClock::CPU_FREQUENCY_GHZ << " GHz\n";
    std::cout << "=======================================================\n";
    
    test_sequential(FIB_N, TOTAL_TASKS);
    test_threads(FIB_N, TOTAL_TASKS);
    test_coroutines(FIB_N, TOTAL_TASKS);
    test_coroutines_yield(FIB_N, TOTAL_TASKS);
    
    return 0;
}
