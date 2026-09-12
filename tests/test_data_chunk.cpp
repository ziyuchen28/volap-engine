
#include "volap/core/data_chunk.h"
#include "volap/core/flat_vector.h"

#include "test_util.h"

#include <iostream>
#include <vector>
#include <cstdint>

namespace {

using namespace volap::core;


void test_type_metadata()
{
    validate(data_type_v<std::uint8_t> == DataType::Bool8, "uint8_t kind");
    validate(data_type_v<std::int64_t> == DataType::Int64, "int64_t kind");
    validate(data_type_v<float> == DataType::Float32, "float kind");
    validate(data_type_v<double> == DataType::Float64, "double kind");
    succeeded(__func__);
}

void test_data_chunk_empty() 
{
    DataChunk chunk;
    validate(chunk.column_count() == 0, "empty column count");
    validate(chunk.row_count() == 0, "empty row count");
    validate(chunk.empty(), "empty chunk");
    succeeded(__func__);
}

void test_data_chunk_add_empty_columns()
{
    DataChunk chunk;
    chunk.reserve_columns(2);
    chunk.add_column(FlatVector::create(DataType::Int64, 8));
    chunk.add_column(FlatVector::create(DataType::Float64, 8));

    validate(chunk.column_count() == 2, "output column count");
    validate(chunk.row_count() == 0, "output initial row count");
    succeeded(__func__);
}

void test_data_chunk_mismatched_column_size_throws()
{
    auto first = FlatVector::create(DataType::Int64, 4);
    first.set_size(3);

    auto second = FlatVector::create(DataType::Float64, 4);
    second.set_size(2);

    DataChunk chunk;
    chunk.add_column(std::move(first));

    validate_throws<std::invalid_argument>(
        [&] {
            chunk.add_column(std::move(second));
        },
        "mismatched column size"
    );

    validate(chunk.column_count() == 1, "failed add leaves chunk unchanged");
    validate(chunk.row_count() == 3, "failed add leaves row count unchanged");
    succeeded(__func__);
}

void test_data_chunk_write_set_row_count()
{
    DataChunk chunk;
    // id column
    chunk.add_column(FlatVector::create(DataType::Int64, 8));
    // value column
    chunk.add_column(FlatVector::create(DataType::Float64, 8));

    auto *ids = FlatVector::get_mutable_data<std::int64_t>(chunk.column(0));
    auto *values = FlatVector::get_mutable_data<double>(chunk.column(1));

    ids[0] = 101;
    ids[1] = 102;
    ids[2] = 103;

    values[0] = 10.0;
    values[1] = 25.0;
    values[2] = 30.0;

    chunk.set_row_count(3);

    validate(chunk.row_count() == 3, "row count");
    validate(chunk.column(0).size() == 3, "column 0 logical size");
    validate(chunk.column(1).size() == 3, "column 1 logical size");

    validate(ids[0] == 101, "id 0");
    validate(ids[2] == 103, "id 2");
    validate(values[0] == 10.0, "value 0");
    validate(values[2] == 30.0, "value 2");
    succeeded(__func__);
}

void test_data_chunk_row_count_over_vec_capacity()
{
    DataChunk chunk;
    chunk.add_column(FlatVector::create(DataType::Int64, 8));
    chunk.add_column(FlatVector::create(DataType::Float64, 2));

    validate_throws<std::out_of_range>(
        [&] {
            chunk.set_row_count(3);
        },
        "row count over one column capacity throws"
    );

    validate(chunk.row_count() == 0, "failed row count preserves chunk");
    validate(chunk.column(0).size() == 0, "column 0 not partially updated");
    validate(chunk.column(1).size() == 0, "column 1 not partially updated");
    succeeded(__func__);
}

void test_data_chunk_clear_preserves_columns_buffers_and_capacity()
{
    DataChunk chunk;
    chunk.add_column(FlatVector::create(DataType::Int64, 1024));
    chunk.add_column(FlatVector::create(DataType::Float64, 1024));

    auto *ids_before = FlatVector::get_mutable_data<std::int64_t>(chunk.column(0));
    auto *values_before = FlatVector::get_mutable_data<double>(chunk.column(1));

    ids_before[0] = 101;
    values_before[0] = 25.0;

    chunk.set_row_count(1);
    chunk.clear();

    validate(chunk.row_count() == 0, "clear resets row count");
    validate(chunk.column_count() == 2, "clear retains columns");
    validate(chunk.column(0).size() == 0, "clear resets column 0 size");
    validate(chunk.column(1).size() == 0, "clear resets column 1 size");

    validate(chunk.column(0).capacity() == 1024,
         "clear retains column 0 capacity");
    validate(chunk.column(1).capacity() == 1024,
         "clear retains column 1 capacity");

    auto *ids_after = FlatVector::get_mutable_data<std::int64_t>(chunk.column(0));
    auto *values_after = FlatVector::get_mutable_data<double>(chunk.column(1));

    validate(ids_after == ids_before, "clear reuses id buffer");
    validate(values_after == values_before, "clear reuses value buffer");
    succeeded(__func__);
}

void test_mixed_owned_and_external_columns()
{
    auto external_owner = std::make_shared<std::vector<std::int64_t>>(
            std::initializer_list<std::int64_t>{101, 102, 103});

    auto external_ids = FlatVector::wrap_external(DataType::Int64,
                                                  external_owner->data(),
                                                  external_owner->size(),
                                                  external_owner);

    auto owned_values = FlatVector::create(DataType::Float64, 3);
    auto *values = FlatVector::get_mutable_data<double>(owned_values);

    values[0] = 10.0;
    values[1] = 20.0;
    values[2] = 30.0;
    owned_values.set_size(3);

    DataChunk chunk;
    chunk.add_column(std::move(external_ids));
    chunk.add_column(std::move(owned_values));

    validate(chunk.row_count() == 3, "mixed ownership row count");
    validate(!chunk.column(0).is_writable(), "external column is read-only");
    validate(chunk.column(1).is_writable(), "owned column remains writable");
    succeeded(__func__);
}

void test_move_data_chunk()
{
    DataChunk source;
    source.add_column(FlatVector::create(DataType::Int64, 4));
    source.set_row_count(2);

    DataChunk destination = std::move(source);
    validate(destination.column_count() == 1, "moved destination columns");
    validate(destination.row_count() == 2, "moved destination row count");

    validate(source.column_count() == 0, "moved source columns reset");
    validate(source.row_count() == 0, "moved source row count reset");
    succeeded(__func__);
}


} // namespace


int main()
{
    test_type_metadata();
    test_data_chunk_empty();
    test_data_chunk_add_empty_columns();
    test_data_chunk_mismatched_column_size_throws();
    test_data_chunk_write_set_row_count();
    test_data_chunk_row_count_over_vec_capacity();
    test_data_chunk_clear_preserves_columns_buffers_and_capacity();
    test_mixed_owned_and_external_columns();
    test_move_data_chunk();
    std::cout << "test_data_chunk: PASS\n";
    return 0;
}

