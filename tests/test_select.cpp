
#include "volap/kernels/select.h"
#include "volap/core/flat_vector.h"

#include "test_util.h"

namespace {

using namespace volap::kernels;
using namespace volap::core;

template <typename T>
Vector make_flat_vector(std::initializer_list<T> values)
{
    Vector vector = FlatVector::create(data_type_v<T>, values.size());

    T *data = FlatVector::get_mutable_data<T>(vector);
    std::copy(values.begin(), values.end(), data);
    vector.set_size(values.size());

    return vector;
}

void test_select_i64_greater_than()
{
    auto input = make_flat_vector<std::int64_t>(
        {10, 200, 30, 400}
    );

    SelectionVector out;
    select_i64_gt(input, 100, out);

    validate(out.size() == 2, "i64 gt size");
    validate(out[0] == 1, "i64 gt index 0");
    validate(out[1] == 3, "i64 gt index 1");

    succeeded(__func__);
}

void test_select_f64_greater_than()
{
    auto input = make_flat_vector<double>(
        {1.0, 2.5, -5.5, 9.9}
    );

    SelectionVector out;
    select_f64_gt(input, 2.0, out);

    validate(out.size() == 2, "f64 gt size");
    validate(out[0] == 1, "f64 gt index 0");
    validate(out[1] == 3, "f64 gt index 1");

    succeeded(__func__);
}

void test_select_f32_greater_than()
{
    auto input = make_flat_vector<float>(
        {1.0f, 2.5f, -5.5f, 9.9f}
    );

    SelectionVector out;
    select_f32_gt(input, 2.0f, out);

    validate(out.size() == 2, "f32 gt size");
    validate(out[0] == 1, "f32 gt index 0");
    validate(out[1] == 3, "f32 gt index 1");

    succeeded(__func__);
}


void test_type_mismatch_throw()
{
    auto input = make_flat_vector<float>(
        {1.0f, 2.5f, -5.5f, 9.9f}
    );

    SelectionVector out;

    bool threw = false;
    try {
        select_i64_gt(input, 0, out);
    } catch (const std::logic_error&) {
        threw = true;
    }

    validate(threw, "select type mismatch throws");
}


} // anynomous namespace


int main(int argc, char **argv)
{
    test_select_i64_greater_than();
    test_select_f64_greater_than();
    test_select_f32_greater_than();
    test_type_mismatch_throw();
}
