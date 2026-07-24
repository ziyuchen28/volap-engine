
#include "volap/core/type.h"
#include "volap/core/column_view.h"
#include "volap/core/data_chunk.h"
#include "volap/core/selection_vector.h"

#include <iostream>
#include <vector>
#include <cstdint>

namespace {

using namespace volap::core;

using volap::core::Type;
using volap::core::ColumnView;
using volap::core::DataChunk;
using volap::core::SelectionVector;

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "[Check failed]: " << message << "\n";
        std::exit(1);
    }
}

void test_type_metadata()
{
    check(type_v<std::uint8_t> == Type::Bool8, "uint8_t kind");
    check(type_v<std::int64_t> == Type::Int64, "int64_t kind");
    check(type_v<float> == Type::Float32, "float kind");
    check(type_v<double> == Type::Float64, "double kind");
}

void test_column_view_int64()
{
    std::vector<std::int64_t> values {10, 20, 30, 40, 50, 60};

    ColumnView column(values.data(), values.size());
    check(column.type() == Type::Int64, "ColumnView int64 type");
    check(column.row_count() == 6, "ColumnView row_count");
    check(column.byte_size() == 6 * sizeof(std::int64_t), "ColumnView byte_size");

    auto span = column.as_span<std::int64_t>();
    check(span.size() == 6, "ColumnView int64 span size");
    check(span[0] == 10, "ColumnView int64 value at index 0");
    check(span[1] == 20, "ColumnView int64 value at index 1");
    check(span[2] == 30, "ColumnView int64 value at index 2");
}

void test_column_view_float32()
{
    std::vector<float> values {1.5f, 2.5f, 3.5f};

    ColumnView column(values.data(), values.size());
    check(column.type() == Type::Float32, "ColumnView float32 type");
    check(column.row_count() == 3, "ColumnView float32 row_count");

    auto span = column.as_span<float>();
    check(span.size() == 3, "ColumnView float32 span size");
    check(span[0] == 1.5f, "ColumnView float32 value at index 0");
    check(span[1] == 2.5f, "ColumnView float32 value at index 1");
    check(span[2] == 3.5f, "ColumnView float32 value at index 2");
}

void test_column_view_type_mismatch_throws()
{
    std::vector<float> values {1.0f, 2.0f};

    ColumnView column(values.data(), values.size());
    bool threw = false;
    try {
        (void)column.as_span<std::int64_t>();
    } catch (const std::logic_error&) {
        threw = true;
    }
    check(threw, "ColumnView type mismatch throws");
}

void test_column_view_non_empty_null_throws()
{
    bool threw = false;
    try {
        ColumnView column(Type::Int64, nullptr, 1);
        (void)column;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "ColumnView data null throws");
}


void test_column_view_empty_null()
{
    ColumnView column(Type::Int64, nullptr, 0);
    check(column.row_count() == 0, "Empty null ColumnView row_count");
    check(column.empty(), "Empty null ColumnView empty");
}

void test_data_chunk_same_length_columns()
{
    std::vector<std::int64_t> ids {1, 2, 3};
    std::vector<double> values {10.0, 20.0, 30.0};

    DataChunk chunk;
    chunk.add_column(ColumnView(ids.data(), ids.size()));
    chunk.add_column(ColumnView(values.data(), values.size()));
    check(chunk.column_count() == 2, "DataChunk column_count");
    check(chunk.row_count() == 3, "DataChunk row_count");

    auto id_span = chunk.column(0).as_span<std::int64_t>();
    auto value_span = chunk.column(1).as_span<double>();

    check(id_span[0] == 1, "DataChunk id at index 0");
    check(value_span[2] == 30.0, "DataChunk value at index 2");
}


void test_data_chunk_mismatched_lengths_throw()
{
    std::vector<std::int64_t> ids {1, 2, 3};
    std::vector<double> value {10.0, 20.0};

    bool threw = false;
    try {
        volap::core::DataChunk chunk;
        chunk.add_column(volap::core::ColumnView(ids.data(), ids.size()));
        chunk.add_column(volap::core::ColumnView(value.data(), value.size()));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "DataChunk mismatched lengths throw");
}

void test_data_chunk_empty()
{
    volap::core::DataChunk chunk;
    check(chunk.column_count() == 0, "DataChunk column_count");
    check(chunk.row_count() == 0, "DataChunk row_count");
    check(chunk.empty(), "DataChunk empty");
}

void test_selection_vector()
{
    SelectionVector sel;

    sel.reserve(4);
    sel.push_back(3);
    sel.push_back(7);
    sel.push_back(11);

    check(sel.size() == 3, "SelectionVector size");
    check(sel[0] == 3, "SelectionVector index 0");
    check(sel[1] == 7, "SelectionVector index 1");
    check(sel[2] == 11, "SelectionVector index 2");

    auto span = sel.as_span();
    check(span.size() == 3, "SelectionVector span size");
    check(span[2] == 11, "SelectionVector span value");

    sel.clear();
    check(sel.empty(), "SelectionVector clear");
}

void test_selection_vector_overflow_throws()
{
    SelectionVector sel;

    bool threw = false;
    try {
        sel.push_back(static_cast<std::size_t>(U32_MAX) + 1);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw, "SelectionVector add out of range");
}


} // namespace


int main()
{
    test_type_metadata();
    test_column_view_int64();
    test_column_view_float32();
    test_column_view_type_mismatch_throws();
    test_column_view_non_empty_null_throws();
    test_column_view_empty_null();
    test_data_chunk_same_length_columns();
    test_data_chunk_mismatched_lengths_throw();
    test_selection_vector();
    test_selection_vector_overflow_throws();
    std::cout << "test_data_chunk: PASS\n";
    return 0;
}

