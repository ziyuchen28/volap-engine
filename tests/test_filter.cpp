
#include "volap/core/data_chunk.h"
#include "volap/execution/filter.h"
#include "volap/execution/in_memory_scan.h"

#include "test_util.h"

namespace {

using namespace volap::core;
using namespace volap::execution;

DataChunk make_input_chunk()
{
    DataChunk input;

    input.add_column(
        make_flat_vector<std::int64_t>({
            101, 102, 103, 104
        })
    );

    input.add_column(
        make_flat_vector<double>({
            10.0, 25.0, 40.0, 15.0
        })
    );

    return input;
}


void test_f64_filter_selected_rows()
{
    DataChunk input = make_input_chunk();

    Filter filter = Filter::f64_greater_than(1, 20.0);

    DataChunk output;
    filter.execute(input, output);

    validate(output.column_count() == 2, "output column count");
    validate(output.row_count() == 2, "output row count");

    const auto *ids = FlatVector::get_data<std::int64_t>( output.column(0));

    const auto *values = FlatVector::get_data<double>( output.column(1));

    validate(ids[0] == 102, "selected id 0");
    validate(ids[1] == 103, "selected id 1");

    validate(values[0] == 25.0, "selected price 0");
    validate(values[1] == 40.0, "selected price 1");

    succeeded(__func__);

}


void test_filter_no_matches()
{
    DataChunk input = make_input_chunk();

    Filter filter = Filter::f64_greater_than(1, 1000.0);

    DataChunk output;
    filter.execute(input, output);

    validate(output.column_count() == 2, "no-match schema retained");
    validate(output.row_count() == 0, "no-match row count");
    validate(filter.selection().empty(), "no-match selection");

    succeeded(__func__);
}


void test_filter_reuses_output_buffers()
{
    DataChunk input = make_input_chunk();

    Filter filter = Filter::f64_greater_than(1, 20.0);

    DataChunk reusable_output;

    filter.execute(input, reusable_output);

    const auto *first_id_addr =
        FlatVector::get_data<std::int64_t>(
            reusable_output.column(0)
        );

    const auto *first_value_addr =
        FlatVector::get_data<double>(
            reusable_output.column(1)
        );

    filter.execute(input, reusable_output);

    const auto *second_id_addr =
        FlatVector::get_data<std::int64_t>(
            reusable_output.column(0)
        );

    const auto *second_value_addr =
        FlatVector::get_data<double>(
            reusable_output.column(1)
        );

    validate(
        first_id_addr == second_id_addr,
        "id output buffer reused"
    );

    validate(
        first_value_addr == second_value_addr,
        "value output buffer reused"
    );

    succeeded(__func__);
}

void test_filter_type_mismatch_throws()
{
    DataChunk input = make_input_chunk();

    // Column 1 is Float64, not Int64.
    Filter filter = Filter::i64_greater_than(1, 20);

    bool threw = false;
    DataChunk output;
    try {
        filter.execute(input, output);
    } catch (const std::logic_error&) {
        threw = true;
    }

    validate(threw, "predicate type mismatch throws");

    succeeded(__func__);
}

void test_scan_filter_pipeline()
{
    DataChunk source;

    source.add_column(
        make_flat_vector<std::int64_t>({
            101,
            102,
            103,
            104,
            105,
            106
        })
    );

    source.add_column(
        make_flat_vector<double>({
            10.0,
            20.0,
            30.0,
            40.0,
            50.0,
            60.0
        })
    );

    InMemoryScan scan(std::move(source), 2);
    DataChunk scan_output;

    Filter filter = Filter::f64_greater_than(1, 25.0);
    DataChunk filter_output;

    std::vector<std::int64_t> selected_ids;
    selected_ids.reserve(6);

    // 101, 102
    if (scan.next(scan_output)) {
        filter.execute(scan_output, filter_output);
        validate(filter_output.row_count() == 0, "no-match row count");
    }

    // 103, 104
    if (scan.next(scan_output)) {
        filter.execute(scan_output, filter_output);
        validate(filter_output.row_count() == 2, "two matched row count");

        const auto *ids =
            FlatVector::get_data<std::int64_t>(
                filter_output.column(0)
            );

        validate(ids[0] == 103, "selected id");
        validate(ids[1] == 104, "selected id");
    }

    // 105, 106
    if (scan.next(scan_output)) {
        filter.execute(scan_output, filter_output);
        validate(filter_output.row_count() == 2, "two matched row count");

        const auto *ids =
            FlatVector::get_data<std::int64_t>(
                filter_output.column(0)
            );

        validate(ids[0] == 105, "selected id");
        validate(ids[1] == 106, "selected id");
    }

    succeeded(__func__);
}

} // anonymous namespace 

int main()
{
    test_f64_filter_selected_rows();
    test_scan_filter_pipeline();
    test_filter_reuses_output_buffers();
    test_filter_type_mismatch_throws();
    return 0;
}
