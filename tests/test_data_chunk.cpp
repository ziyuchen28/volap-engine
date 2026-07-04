
#include "volap/core/type.h"
#include "volap/core/column_view.h"

#include <iostream>
#include <vector>
#include <cstdint>

namespace {

using namespace volap::core;
using volap::core::ColumnView;
using volap::core::Type;

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
    check(span[0] == 10, "ColumnView int64 value 0");
    check(span[1] == 20, "ColumnView int64 value 1");
    check(span[2] == 30, "ColumnView int64 value 2");
}

void test_column_view_float32()
{
    std::vector<float> values {1.5f, 2.5f, 3.5f};

    ColumnView column(values.data(), values.size());
    check(column.type() == Type::Float32, "ColumnView float32 type");
    check(column.row_count() == 3, "ColumnView float32 row_count");

    auto span = column.as_span<float>();
    check(span.size() == 3, "ColumnView float32 span size");
    check(span[0] == 1.5f, "ColumnView float32 value 0");
    check(span[1] == 2.5f, "ColumnView float32 value 1");
    check(span[2] == 3.5f, "ColumnView float32 value 2");
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


} // namespace



int main()
{
    test_type_metadata();
    test_column_view_int64();
    test_column_view_float32();
    test_column_view_type_mismatch_throws();
    std::cout << "test_data_chunk: PASS\n";
    return 0;
}
