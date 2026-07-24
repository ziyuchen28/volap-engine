
#include "volap/core/flat_vector.h"

#include "test_util.h"

namespace 
{


using namespace volap::core;


void test_flat_vector_metadata()
{
    auto vector = FlatVector::create(Type::Int64, 8);

    validate(vector.data_type() == Type::Int64, "vector data type");
    validate(vector.encoding() == VectorEncoding::Flat, "vector encoding");

    validate(vector.size() == 0, "vector initial size");
    validate(vector.capacity() == 8, "vector capacity");

    validate(vector.is_writable(), "vector is writable");
    std::cout << "[PASS] test_flat_vector_metadata" << std::endl;
}


void test_flat_vector_write_and_read()
{
    auto vector = FlatVector::create(Type::Int64, 8);
    auto *data_mut = FlatVector::get_mutable_data<std::int64_t>(vector);

    data_mut[0] = 10;
    data_mut[1] = 20;
    data_mut[2] = 30;
    vector.set_size(3);

    const auto *data = FlatVector::get_data<std::int64_t>(vector);
    validate(vector.size() == 3, "current size");
    validate(data[0] == 10, "value 0");
    validate(data[1] == 20, "value 1");
    validate(data[2] == 30, "value 2");

    std::cout << "[PASS] test_flat_vector_write_and_read" << std::endl;
}


void test_flat_vector_clear_reuses_allocation()
{
    auto vector = FlatVector::create(Type::Float64, 4);
    auto *initial_data = FlatVector::get_mutable_data<double>(vector);
    const double *initial_address = initial_data;

    initial_data[0] = 1.5;
    initial_data[1] = 5.05;
    initial_data[2] = 9.99;
    vector.set_size(3);

    vector.clear();
    validate(vector.size() == 0, "clear resets size");
    validate(vector.capacity() == 4, "clear preserves capacity");

    auto *reused_data = FlatVector::get_mutable_data<double>(vector);
    validate(
        reused_data == initial_address,
        "clear preserves allocation"
    );

    reused_data[0] = 100.0;
    vector.set_size(1);

    validate(
        FlatVector::get_data<double>(vector)[0] == 100.0,
        "allocation reusable after clear"
    );

    std::cout << "[PASS] test_flat_vector_clear_reuses_allocation" << std::endl;
}

} // namespace


int main() 
{
    test_flat_vector_metadata();
    test_flat_vector_write_and_read();
    test_flat_vector_clear_reuses_allocation();
}
