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
Context Switch Overhead Comparison
Configuration:
  Fibonacci N:      25
  Total tasks:      1000
  CPU frequency:    2.8 GHz
=======================================================

=== Sequential (No Context Switch) ===
  Total time:       892304 ticks (318680 ns)
  Avg per task:     892 ticks (318.571 ns)
  Throughput:       3.13794e+06 tasks/sec

=== Thread-based (1000 threads, one per task) ===
  Total time:       1467181174 ticks (5.23993e+08 ns)
  Creation time:    1291773918 ticks (4.61348e+08 ns, 88.0446%)
  Compute+Join time:59885736 ticks (2.13878e+07 ns, 4.08169%) - Parallel execution + cleanup
  Avg per task:     1467181 ticks (523993 ns)
  Throughput:       1908.42 tasks/sec
  Avg creation:     1291773 ticks (461348 ns)

=== Coroutine-based ===
  Total time:       885217868 ticks (3.16149e+08 ns)
  Creation overhead: 51002 ticks (18215 ns, 0.00576152%)
  Destruction overhead: 93456 ticks (33377.1 ns, 0.0105574%)
  Compute time:     884848402 ticks (3.16017e+08 ns, 99.9583%)
  Avg per task:     885217 ticks (316149 ns)
  Throughput:       3163.06 tasks/sec
  Avg creation:     51 ticks (18.2143 ns)
  Avg destruction:  93 ticks (33.2143 ns)

=== Coroutine-based with co_yield ===
  Total time:          881081370 ticks (3.14672e+08 ns)
  Creation overhead:   52 ticks (18.5714 ns, 5.90184e-06%)
  Destruction overhead:0 ticks (0 ns, 0%)
  --- Round-trip measurements ---
  Compute time:   880926120 ticks (3.14616e+08 ns, 99.9824%) (Measured inside coroutine)
  Switch overhead: 102152 ticks (36482.9 ns, 0.0115939%) (Round-trip - Compute)
  Avg per task:        881081 ticks (314672 ns)
  Throughput:          3177.91 tasks/sec
  Yield/Resume count:  1000
  Avg round-trip:      881028 ticks (314653 ns)
  Avg pure compute:    880926 ticks (314616 ns)
  Avg switch overhead: 102 ticks (36.4286 ns) (This is the co_yield/resume cost)
```

## Results with Single CPU Core (taskset -c 0)

```
Context Switch Overhead Comparison
Configuration:
  Fibonacci N:      25
  Total tasks:      1000
  CPU frequency:    2.8 GHz
=======================================================

=== Sequential (No Context Switch) ===
  Total time:       894644 ticks (319516 ns)
  Avg per task:     894 ticks (319.286 ns)
  Throughput:       3.12974e+06 tasks/sec

=== Thread-based (1000 threads, one per task) ===
  Total time:       33453039890 ticks (1.19475e+10 ns)
  Creation time:    32451607150 ticks (1.15899e+10 ns, 97.0065%)
  Compute+Join time:940963868 ticks (3.36059e+08 ns, 2.81279%) - Parallel execution + cleanup
  Avg per task:     33453039 ticks (1.19475e+07 ns)
  Throughput:       83.6994 tasks/sec
  Avg creation:     32451607 ticks (1.15899e+07 ns)

=== Coroutine-based ===
  Total time:       884945426 ticks (3.16052e+08 ns)
  Creation overhead: 50950 ticks (18196.4 ns, 0.00575742%)
  Destruction overhead: 63596 ticks (22712.9 ns, 0.00718643%)
  Compute time:     884625992 ticks (3.15938e+08 ns, 99.9639%)
  Avg per task:     884945 ticks (316052 ns)
  Throughput:       3164.04 tasks/sec
  Avg creation:     50 ticks (17.8571 ns)
  Avg destruction:  63 ticks (22.5 ns)

=== Coroutine-based with co_yield ===
  Total time:          881548560 ticks (3.14839e+08 ns)
  Creation overhead:   50 ticks (17.8571 ns, 5.67184e-06%)
  Destruction overhead:0 ticks (0 ns, 0%)
  --- Round-trip measurements ---
  Compute time:   881393808 ticks (3.14784e+08 ns, 99.9824%) (Measured inside coroutine)
  Switch overhead: 102062 ticks (36450.7 ns, 0.0115776%) (Round-trip - Compute)
  Avg per task:        881548 ticks (314839 ns)
  Throughput:          3176.23 tasks/sec
  Yield/Resume count:  1000
  Avg round-trip:      881495 ticks (314820 ns)
  Avg pure compute:    881393 ticks (314783 ns)
  Avg switch overhead: 102 ticks (36.4286 ns) (This is the co_yield/resume cost)
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