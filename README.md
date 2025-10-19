# C++ Coroutine & Thread Performance Benchmark

This benchmark compares the performance of sequential execution, OS threads, and C++20 coroutines for concurrent tasks.

## Test Configuration

Calculate Fibonacci(30) using recursive algorithm, using clang++-21 with -O3 optimization

## Measurement Method

### Thread Measurement
- Creation time: Time to create all threads
- Compute+Join time: Time from starting all threads to join completion (parallel execution + cleanup)

### Coroutine Measurement
- Uses `suspend_always` at `initial_suspend()` to separate creation from execution
- Creation overhead: Measured from `promise_type()` constructor to `get_return_object()` completion
- Destruction overhead: Measured in `~Task()` destructor calling `handle.destroy()`
- Compute time: Measured explicitly during `resume()` calls

## Results with All CPU Cores (128 cores)

```
=== Sequential (No Context Switch) ===
  Total time:       9783118 ticks (3.49397e+06 ns) [3.49 ms]
  Avg per task:     9783 ticks (3493.93 ns)
  Throughput:       286207 tasks/sec

=== Thread-based (1000 threads, one per task) ===
  Total time:       1426221886 ticks (5.09365e+08 ns) [509 ms]
  Creation time:    1177178750 ticks (4.20421e+08 ns, 82.5%)
  Compute+Join time:159147666 ticks (5.68385e+07 ns, 11.2%) - Parallel execution + cleanup
  Avg per task:     1426221 ticks (509365 ns)
  Throughput:       1963.23 tasks/sec
  Avg creation:     1177178 ticks (420421 ns) [420 μs per thread]

=== Coroutine-based ===
  Total time:       9771279006 ticks (3.48974e+09 ns) [3.49 seconds]
  Creation overhead: 50854 ticks (18162.1 ns, 0.00052%)
  Destruction overhead: 117676 ticks (42027.1 ns, 0.0012%)
  Compute time:     9770862932 ticks (3.48959e+09 ns, 99.996%)
  Avg per task:     9771279 ticks (3.48974e+06 ns)
  Throughput:       286.554 tasks/sec
  Avg creation:     50 ticks (18 ns)
  Avg destruction:  117 ticks (42 ns)
```

## Results with Single CPU Core (taskset -c 0)

```
=== Sequential (No Context Switch) ===
  Total time:       9777578 ticks (3.49199e+06 ns) [3.49 ms]
  Avg per task:     9777 ticks (3491.79 ns)
  Throughput:       286369 tasks/sec

=== Thread-based (1000 threads, one per task) ===
  Total time:       41657587134 ticks (1.48777e+10 ns) [14.9 seconds]
  Creation time:    31787100820 ticks (1.13525e+10 ns, 76.3%)
  Compute+Join time:9814475276 ticks (3.50517e+09 ns, 23.6%) - Parallel execution + cleanup
  Avg per task:     41657587 ticks (1.48777e+07 ns)
  Throughput:       67.2146 tasks/sec
  Avg creation:     31787100 ticks (1.13525e+07 ns) [11.4 ms per thread]

=== Coroutine-based ===
  Total time:       9772636610 ticks (3.49023e+09 ns) [3.49 seconds]
  Creation overhead: 50932 ticks (18190 ns, 0.00052%)
  Destruction overhead: 110042 ticks (39300.7 ns, 0.0011%)
  Compute time:     9772291950 ticks (3.4901e+09 ns, 99.996%)
  Avg per task:     9772636 ticks (3.49023e+06 ns)
  Throughput:       286.514 tasks/sec
  Avg creation:     50 ticks (18 ns)
  Avg destruction:  110 ticks (40 ns)
```

## Running the Benchmark

```bash
clang++-21 -std=c++23 -O3 coroutine3.cpp -o coroutine3

# Run with all CPU cores
./coroutine3

# Run with single CPU core
taskset -c 0 ./coroutine3
```

## TODOs
- [ ] The creation and destruction time of coroutine is faster than expected, check if it is correct