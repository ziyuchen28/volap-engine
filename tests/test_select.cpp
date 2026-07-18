
#include "volap/kernels/select.h"

#include <vector>
#include <iostream>

namespace {

using namespace volap::kernels;
using namespace volap::core;

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "CHECK failed: " << message << "\n";
        std::exit(1);
    }
}

void test_select_i64_greater_than()
{
    std::vector<std::int64_t> values {10, 200, 50, 300};

    ColumnView column(values.data(), values.size());
    SelectionVector out;

    select_i64_gt(column, 100, out);

    check(out.size() == 2, "i64 gt size");
    check(out[0] == 1, "i64 gt index 0");
    check(out[1] == 3, "i64 gt index 1");
}


void test_select_f64_greater_than()
{
    std::vector<double> values {1.0, 2.5, -5.5, 9.9};

    ColumnView column(values.data(), values.size());
    SelectionVector out;

    select_i64_gt(column, 2.0, out);

    check(out.size() == 2, "f64 gt size");
    check(out[0] == 1, "f64 gt index 0");
    check(out[1] == 3, "f64 gt index 1");
}


void test_select_f32_greater_than()
{
    std::vector<float> values {1.0f, 2.5f, -5.5f, 9.9f};

    ColumnView column(values.data(), values.size());
    SelectionVector out;

    select_i64_gt(column, 2.0, out);

    check(out.size() == 2, "f32 gt size");
    check(out[0] == 1, "f32 gt index 0");
    check(out[1] == 3, "f32 gt index 1");
}


void test_type_mismatch_throw()
{
    std::vector<float> values {1.0f, 2.0f};

    ColumnView column(values.data(), values.size());
    SelectionVector out;

    bool threw = false;
    try {
        select_i64_gt(column, 0, out);
    } catch (const std::logic_error&) {
        threw = true;
    }

    check(threw, "select type mismatch throws");
}


} // namespace


int main(int argc, char **argv)
{
    test_select_i64_greater_than();
    test_select_f64_greater_than();
    test_select_f32_greater_than();
}
