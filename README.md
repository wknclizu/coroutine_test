# C++ Coroutine & Thread Performance Benchmark

This benchmark compares the performance of sequential execution, OS threads, and C++20 coroutines for concurrent tasks.

## Test Configuration

Calculate Fibonacci(n) using recursive algorithm, using clang++-21 and g++ 11.4.0 with -O3 optimization

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
### Using clang++-21 with -O3 optimization
```
Context Switch Overhead Comparison
Configuration:
  Fibonacci N:      25
  Total tasks:      1000
  CPU frequency:    2.8 GHz
=======================================================

=== Sequential (No Context Switch) ===
  Total time:       1297148 ticks (463267 ns)
  Avg per task:     1297 ticks (463.214 ns)
  Throughput:       2.15858e+06 tasks/sec

=== Thread-based (1000 threads, one per task) ===
  Total time:       1340585890 ticks (4.78781e+08 ns)
  Creation time:    1186057436 ticks (4.23592e+08 ns, 88.4731%)
  Compute time:      52402622 ticks (1.87152e+07 ns, 3.90893%) - Parallel execution
  Avg per task:     1340585 ticks (478780 ns)
  Throughput:       2088.64 tasks/sec
  Avg creation:     1186057 ticks (423592 ns)

=== Coroutine-based ===
  Total time:       882138854 ticks (3.1505e+08 ns)
  Creation overhead: 50260 ticks (17950 ns, 0.00569752%)
  Destruction overhead: 88850 ticks (31732.1 ns, 0.0100721%)
  Resume time:      881803440 ticks (3.1493e+08 ns, 99.962%) - Includes switch + compute
  Compute time:      881707604 ticks (3.14896e+08 ns, 99.9511%) - Measured inside coroutine
  Switch overhead:  95836 ticks (34227.1 ns, 0.010864%) - Resume time - Compute time
  Avg per task:     882138 ticks (315049 ns)
  Throughput:       3174.1 tasks/sec
  Avg creation:     50 ticks (17.8571 ns)
  Avg destruction:  88 ticks (31.4286 ns)
  Avg switch overhead: 95 ticks (33.9286 ns)

=== Coroutine-based with co_yield ===
  Total time:       880920756 ticks (3.14615e+08 ns)
  Creation overhead: 50 ticks (17.8571 ns, 5.67588e-06%)
  Destruction overhead: 0 ticks (0 ns, 0%)
  Resume time:      880867908 ticks (3.14596e+08 ns, 99.994%) - Includes switch + compute
  Compute time:      880751322 ticks (3.14554e+08 ns, 99.9808%) - Measured inside coroutine
  Switch overhead:  116586 ticks (41637.9 ns, 0.0132346%) - Resume time - Compute time
  Avg per task:     880920 ticks (314614 ns)
  Throughput:       3178.49 tasks/sec
  Yield/Resume count:  1000
  Avg resume time:  880867 ticks (314595 ns)
  Avg pure compute: 880751 ticks (314554 ns)
  Avg switch overhead: 116 ticks (41.4286 ns)
```
### Using g++ 11.4.0 with -O3 optimization
```
Context Switch Overhead Comparison
Configuration:
  Fibonacci N:      25
  Total tasks:      1000
  CPU frequency:    2.8 GHz
=======================================================

=== Sequential (No Context Switch) ===
  Total time:       340824726 ticks (1.21723e+08 ns)
  Avg per task:     340824 ticks (121723 ns)
  Throughput:       8215.37 tasks/sec

=== Thread-based (1000 threads, one per task) ===
  Total time:       967809752 ticks (3.45646e+08 ns)
  Creation time:    795401368 ticks (2.84072e+08 ns, 82.1857%)
  Compute time:      46667768 ticks (1.66671e+07 ns, 4.822%) - Parallel execution
  Avg per task:     967809 ticks (345646 ns)
  Throughput:       2893.13 tasks/sec
  Avg creation:     795401 ticks (284072 ns)

=== Coroutine-based ===
  Total time:       328750402 ticks (1.17411e+08 ns)
  Creation overhead: 50268 ticks (17952.9 ns, 0.0152906%)
  Destruction overhead: 95460 ticks (34092.9 ns, 0.0290372%)
  Resume time:      328400740 ticks (1.17286e+08 ns, 99.8936%) - Includes switch + compute
  Compute time:      328293364 ticks (1.17248e+08 ns, 99.861%) - Measured inside coroutine
  Switch overhead:  107376 ticks (38348.6 ns, 0.0326619%) - Resume time - Compute time
  Avg per task:     328750 ticks (117411 ns)
  Throughput:       8517.1 tasks/sec
  Avg creation:     50 ticks (17.8571 ns)
  Avg destruction:  95 ticks (33.9286 ns)
  Avg switch overhead: 107 ticks (38.2143 ns)

=== Coroutine-based with co_yield ===
  Total time:       327510326 ticks (1.16968e+08 ns)
  Creation overhead: 76 ticks (27.1429 ns, 2.32054e-05%)
  Destruction overhead: 0 ticks (0 ns, 0%)
  Resume time:      327457364 ticks (1.16949e+08 ns, 99.9838%) - Includes switch + compute
  Compute time:      327351586 ticks (1.16911e+08 ns, 99.9515%) - Measured inside coroutine
  Switch overhead:  105778 ticks (37777.9 ns, 0.0322976%) - Resume time - Compute time
  Avg per task:     327510 ticks (116968 ns)
  Throughput:       8549.35 tasks/sec
  Yield/Resume count:  1000
  Avg resume time:  327457 ticks (116949 ns)
  Avg pure compute: 327351 ticks (116911 ns)
  Avg switch overhead: 105 ticks (37.5 ns)
```

## Results with Single CPU Core (taskset -c 0)
### Using clang++-21 with -O3 optimization
```
Context Switch Overhead Comparison
Configuration:
  Fibonacci N:      25
  Total tasks:      1000
  CPU frequency:    2.8 GHz
=======================================================

=== Sequential (No Context Switch) ===
  Total time:       1289102 ticks (460394 ns)
  Avg per task:     1289 ticks (460.357 ns)
  Throughput:       2.17205e+06 tasks/sec

=== Thread-based (1000 threads, one per task) ===
  Total time:       34668932446 ticks (1.23818e+10 ns)
  Creation time:    33694314754 ticks (1.20337e+10 ns, 97.1888%)
  Compute time:      915583230 ticks (3.26994e+08 ns, 2.64093%) - Parallel execution
  Avg per task:     34668932 ticks (1.23818e+07 ns)
  Throughput:       80.764 tasks/sec
  Avg creation:     33694314 ticks (1.20337e+07 ns)

=== Coroutine-based ===
  Total time:       884235216 ticks (3.15798e+08 ns)
  Creation overhead: 50204 ticks (17930 ns, 0.00567767%)
  Destruction overhead: 89468 ticks (31952.9 ns, 0.0101181%)
  Resume time:      883910708 ticks (3.15682e+08 ns, 99.9633%) - Includes switch + compute
  Compute time:      883815112 ticks (3.15648e+08 ns, 99.9525%) - Measured inside coroutine
  Switch overhead:  95596 ticks (34141.4 ns, 0.0108112%) - Resume time - Compute time
  Avg per task:     884235 ticks (315798 ns)
  Throughput:       3166.58 tasks/sec
  Avg creation:     50 ticks (17.8571 ns)
  Avg destruction:  89 ticks (31.7857 ns)
  Avg switch overhead: 95 ticks (33.9286 ns)

=== Coroutine-based with co_yield ===
  Total time:       881470244 ticks (3.14811e+08 ns)
  Creation overhead: 52 ticks (18.5714 ns, 5.89923e-06%)
  Destruction overhead: 0 ticks (0 ns, 0%)
  Resume time:      881417462 ticks (3.14792e+08 ns, 99.994%) - Includes switch + compute
  Compute time:      881300750 ticks (3.1475e+08 ns, 99.9808%) - Measured inside coroutine
  Switch overhead:  116712 ticks (41682.9 ns, 0.0132406%) - Resume time - Compute time
  Avg per task:     881470 ticks (314811 ns)
  Throughput:       3176.51 tasks/sec
  Yield/Resume count:  1000
  Avg resume time:  881417 ticks (314792 ns)
  Avg pure compute: 881300 ticks (314750 ns)
  Avg switch overhead: 116 ticks (41.4286 ns)
```
### Using g++ 11.4.0 with -O3 optimization
```
Context Switch Overhead Comparison
Configuration:
  Fibonacci N:      25
  Total tasks:      1000
  CPU frequency:    2.8 GHz
=======================================================

=== Sequential (No Context Switch) ===
  Total time:       343141398 ticks (1.2255e+08 ns)
  Avg per task:     343141 ticks (122550 ns)
  Throughput:       8159.9 tasks/sec

=== Thread-based (1000 threads, one per task) ===
  Total time:       33195502978 ticks (1.18555e+10 ns)
  Creation time:    32803494966 ticks (1.17155e+10 ns, 98.8191%)
  Compute time:      339660158 ticks (1.21307e+08 ns, 1.02321%) - Parallel execution
  Avg per task:     33195502 ticks (1.18555e+07 ns)
  Throughput:       84.3488 tasks/sec
  Avg creation:     32803494 ticks (1.17155e+07 ns)

=== Coroutine-based ===
  Total time:       326969232 ticks (1.16775e+08 ns)
  Creation overhead: 50202 ticks (17929.3 ns, 0.0153537%)
  Destruction overhead: 80220 ticks (28650 ns, 0.0245344%)
  Resume time:      326648518 ticks (1.1666e+08 ns, 99.9019%) - Includes switch + compute
  Compute time:      326542616 ticks (1.16622e+08 ns, 99.8695%) - Measured inside coroutine
  Switch overhead:  105902 ticks (37822.1 ns, 0.032389%) - Resume time - Compute time
  Avg per task:     326969 ticks (116775 ns)
  Throughput:       8563.5 tasks/sec
  Avg creation:     50 ticks (17.8571 ns)
  Avg destruction:  80 ticks (28.5714 ns)
  Avg switch overhead: 105 ticks (37.5 ns)

=== Coroutine-based with co_yield ===
  Total time:       324529082 ticks (1.15903e+08 ns)
  Creation overhead: 50 ticks (17.8571 ns, 1.54069e-05%)
  Destruction overhead: 0 ticks (0 ns, 0%)
  Resume time:      324476492 ticks (1.15884e+08 ns, 99.9838%) - Includes switch + compute
  Compute time:      324370612 ticks (1.15847e+08 ns, 99.9512%) - Measured inside coroutine
  Switch overhead:  105880 ticks (37814.3 ns, 0.0326257%) - Resume time - Compute time
  Avg per task:     324529 ticks (115903 ns)
  Throughput:       8627.89 tasks/sec
  Yield/Resume count:  1000
  Avg resume time:  324476 ticks (115884 ns)
  Avg pure compute: 324370 ticks (115846 ns)
  Avg switch overhead: 105 ticks (37.5 ns)
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
