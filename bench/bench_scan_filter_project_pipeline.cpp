
#include "benchmark_util.h"

#include "volap/core/data_chunk.h"
#include "volap/core/flat_vector.h"
#include "volap/core/type.h"

#include "volap/execution/in_memory_scan.h"
#include "volap/execution/filter.h"
#include "volap/execution/project.h"

#include <iostream>
#include <random>
#include <chrono>

using namespace volap::core;
using namespace volap::execution;
using namespace volap::bench;

namespace
{

template <typename T>
Vector make_vector(std::size_t row_count)
{
    return FlatVector::create(type_v<T>, row_count);
}

DataChunk make_source(std::size_t row_count)
{
    DataChunk source;

    Vector ids = make_vector<std::int64_t>(row_count);
    Vector qualities = make_vector<double>(row_count);
    Vector quantities = make_vector<double>(row_count);

    std::int64_t *id_data = FlatVector::get_mutable_data<std::int64_t>(ids);
    double *quality_data = FlatVector::get_mutable_data<double>(qualities);
    double *quantity_data = FlatVector::get_mutable_data<double>(quantities);

    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> quality_dist(0.0, 100.0);
    // std::uniform_int_distribution<double> quantity_dist(1, 10);
    std::uniform_real_distribution<double> quantity_dist(1.0, 10.0);

    for (std::size_t row = 0; row < row_count; ++row) {
        id_data[row] = static_cast<std::int64_t>(row);
        quality_data[row] = quality_dist(rng);
        quantity_data[row] = quantity_dist(rng);
    }

    ids.set_size(row_count);
    qualities.set_size(row_count);
    quantities.set_size(row_count);

    source.add_column(std::move(ids));
    source.add_column(std::move(qualities));
    source.add_column(std::move(quantities));

    return source;
}


struct RunResult
{
    std::size_t selected_rows = 0;
    double checksum = 0.0;
};

RunResult run_pipeline(InMemoryScan &scan,
                       Filter &filter,
                       Project &project,
                       DataChunk &scan_output,
                       DataChunk &filter_output,
                       DataChunk &project_output)
{
    scan.reset();

    RunResult result;

    while (scan.next(scan_output)) {
        filter.execute(scan_output, filter_output);
        project.execute(filter_output, project_output);

        const double *total = FlatVector::get_data<double>(project_output.column(1));

        result.selected_rows += project_output.row_count();

        for (std::size_t row = 0;
             row < project_output.row_count();
             ++row) 
        {
            result.checksum += total[row];
        }
    }

    return result;
}

} // anonymous namespace

int main(int argc, char **argv)
{

    std::cout << "benchmark: scan_filter_project scalar\n";

    print_benchmark_environment();

    const std::size_t rows =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--rows", "1048576")));

    const std::size_t chunk_size =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--chunk-size", "2048")));

    const std::size_t iterations =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--iters", "20")));

    const std::size_t warmup =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--warmup", "3")));

    const double threshold = std::stod(get_arg(argc, argv, "--threshold", "50.0"));

    std::cout << "rows: " << rows << '\n';
    std::cout << "chunk_size: " << chunk_size << '\n';
    std::cout << "iterations: " << iterations << '\n';
    std::cout << "threshold: " << threshold << '\n';

    DataChunk source = make_source(rows);
    DataSchema schema = DataSchema::from_chunk(source);

    InMemoryScan scan(std::move(source), chunk_size);
    Filter filter = Filter::f64_greater_than(1, threshold);

    Project project({
        Projection::column(0),
        Projection::multiply(1, 2)
    }, schema);

    DataChunk scan_output;
    DataChunk filter_output;
    DataChunk project_output;

    double checksum = 0.0;
    std::size_t selected_rows = 0;

    // warm up
    for (std::size_t i = 0; i < warmup; ++i) {
        const RunResult result =
            run_pipeline(scan, filter, project, scan_output, filter_output, project_output);

        checksum += result.checksum;
        selected_rows = result.selected_rows;

        do_not_optimize(checksum);
    }

    // actual benchmark
    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < iterations; ++i) {
        const RunResult result =
            run_pipeline(scan, filter, project, scan_output, filter_output, project_output);

        checksum += result.checksum;
        selected_rows = result.selected_rows;

        do_not_optimize(checksum);
    }

    const auto end = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(end - start).count();

    const double total_input_rows = static_cast<double>(rows) * static_cast<double>(iterations);

    const double input_rows_per_sec = total_input_rows / seconds;

    // const double ns_per_input_row = seconds * 1e9 / total_input_rows;

    std::cout << std::fixed << std::setprecision(3);


    std::cout << '\n';
    std::cout << "seconds: " << seconds << '\n';

    std::cout << "throughput(rows/sec): " << input_rows_per_sec << '\n';


    return 0;

}
