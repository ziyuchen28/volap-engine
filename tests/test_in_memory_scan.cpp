
#include "volap/core/data_chunk.h"
#include "volap/core/flat_vector.h"
#include "volap/execution/in_memory_scan.h"

#include "test_util.h"

using namespace volap::execution;

namespace 
{

DataChunk make_source_chunk()
{
    DataChunk source;

    source.add_column(
        make_flat_vector<std::int64_t>({
            101,
            102,
            103,
            104,
            105,
            106,
            107,
            108,
            109,
            110
        })
    );

    source.add_column(
        make_flat_vector<double>({
            10.0,
            20.0,
            30.0,
            40.0,
            50.0,
            60.0,
            70.0,
            80.0,
            90.0,
            100.0
        })
    );

    return source;
}

void validate_output_rows(
    const DataChunk &output,
    std::initializer_list<std::int64_t> expected_ids,
    std::initializer_list<double> expected_values)
{
    validate(
        output.row_count() == expected_ids.size(),
        "output row count"
    );

    validate(
        expected_ids.size() == expected_values.size(),
        "test expected columns have matching size"
    );

    const std::int64_t* ids =
        FlatVector::get_data<std::int64_t>(
            output.column(0)
        );

    const double* values =
        FlatVector::get_data<double>(
            output.column(1)
        );

    std::size_t index = 0;

    for (const std::int64_t expected : expected_ids) {
        validate(
            ids[index] == expected,
            "output id value"
        );

        ++index;
    }

    index = 0;

    for (const double expected : expected_values) {
        validate(
            values[index] == expected,
            "output numeric value"
        );

        ++index;
    }
}

void test_scan_emits_fixed_size_chunks()
{
    DataChunk source = make_source_chunk();

    InMemoryScan scan(std::move(source), 4);
    DataChunk output;

    validate(scan.next(output), "first chunk exists");
    validate_output_rows(
        output,
        {101, 102, 103, 104},
        {10.0, 20.0, 30.0, 40.0}
    );
    validate(scan.position() == 4, "position after first chunk");

    validate(scan.next(output), "second chunk exists");
    validate_output_rows(
        output,
        {105, 106, 107, 108},
        {50.0, 60.0, 70.0, 80.0}
    );
    validate(scan.position() == 8, "position after second chunk");

    validate(scan.next(output), "final partial chunk exists");
    validate_output_rows(
        output,
        {109, 110},
        {90.0, 100.0}
    );
    validate(scan.position() == 10, "position after final chunk");

    validate(
        !scan.next(output),
        "nothing else to scan");

    validate(
        output.row_count() == 0,
        "exhausted scan clears output logical rows");

    succeeded(__func__);
}


void test_scan_reuses_output_buffers()
{
    DataChunk source = make_source_chunk();

    InMemoryScan scan(std::move(source), 4);
    DataChunk output;

    validate(scan.next(output), "first reusable chunk exists");

    const std::int64_t *first_id_address =
        FlatVector::get_data<std::int64_t>(
            output.column(0)
        );

    const double *first_value_address =
        FlatVector::get_data<double>(
            output.column(1)
        );

    validate(scan.next(output), "second reusable chunk exists");

    const std::int64_t *second_id_address =
        FlatVector::get_data<std::int64_t>(
            output.column(0)
        );

    const double *second_value_address =
        FlatVector::get_data<double>(
            output.column(1)
        );

    validate(
        first_id_address == second_id_address,
        "id output buffer reused"
    );

    validate(
        first_value_address == second_value_address,
        "value output buffer reused"
    );

    succeeded(__func__);
}


void test_scan_reset()
{
    DataChunk source = make_source_chunk();

    InMemoryScan scan(std::move(source), 4);
    DataChunk output;

    validate(scan.next(output), "scan before reset");
    validate(scan.position() == 4, "position before reset");

    scan.reset();
    validate(scan.position() == 0, "position reset");

    validate(scan.next(output), "scan after reset");
    validate_output_rows(
        output,
        {101, 102, 103, 104},
        {10.0, 20.0, 30.0, 40.0}
    );

    succeeded(__func__);
}

} // anynomous namespace

int main()
{
    test_scan_emits_fixed_size_chunks();
    return 0;
}
