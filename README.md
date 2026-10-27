# volap-engine

**A columnar query execution engine for analytical workloads.**

VOLAP is a work-in-progress project exploring how analytical queries run close to the hardware: columnar data layout, batch processing, memory ownership, and CPU performance. It focuses on the execution engine rather than a SQL frontend or a complete database.

## Current implementation

- **Columnar storage:** typed flat vectors, aligned owned buffers, externally backed buffers, and reusable `DataChunk` batches.
- **Execution:** an in-memory `Scan → Filter → Project` pipeline with numeric predicates and projections.
- **Binding:** schema-based type resolution, numeric casts, and scratch buffers reused across batches.
- **Testing and benchmarks:** correctness tests and a repeatable single-threaded pipeline benchmark.

The current pipeline processes values in batches. CPU SIMD optimization, grouped aggregation, parallel execution, and disk spill are not part of the main execution path yet.

## Execution model

```text
InMemoryScan → Filter → Project → Result
```

For example, the benchmark runs the equivalent of:

```sql
SELECT id, quality * quantity
FROM input
WHERE quality > 50;
```

The engine currently materializes intermediate batches. Reducing data movement between operators is one of the areas under investigation.

## Build

Currently tested on **Linux x86-64**, including WSL2. Native Windows and macOS builds have not been verified.


```sh
git clone https://github.com/ziyuchen28/volap-engine.git
cd volap-engine

make          # configure and build (RelWithDebInfo by default)
make test     # build and run tests
```

For a Release build, use `make CMAKE_BUILD_TYPE=Release`. Run `make clean` to remove the build directory.
```

## Benchmark

The current benchmark exercises **Scan → Filter → Project** on synthetic in-memory data. It includes the output checksum calculation in the timed section; input generation and warm-up are outside the timed section.

A recorded baseline:

| Setting | Value |
| --- | --- |
| Input | 1,048,576 rows |
| Batch size | 2,048 rows |
| Iterations | 20 timed, 3 warm-up |
| Predicate | `quality > 50` |
| CPU | Intel Core i7-14650HX |
| Environment | x86-64, Linux/WSL2, GCC 13.3, `RelWithDebInfo` |
| Execution | Single-threaded |
| Elapsed time | 0.132 s |
| Throughput | **158.7 million input rows/s** |

Recorded at commit `b906323c5a8e`.

Benchmark source: `bench/bench_scan_filter_project_pipeline.cpp`

```sh
bash bench/bench_scan_filter_project_pipeline.sh
```

## Direction

Planned next batch of work will focus on:

- SIMD-aware predicate selection and reducing unnecessary intermediate materialization.
- Grouped hash aggregation, including table layout, collisions, and memory accounting.
- Parallel execution, bounded memory, and local spill for larger workloads.
- Arrow/Parquet input and more representative analytical queries.

