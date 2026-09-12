
#include "volap/core/flat_vector.h"

#include "test_util.h"

#include <vector>

namespace 
{


using namespace volap::core;


void test_flat_vector_metadata()
{
    auto vector = FlatVector::create(DataType::Int64, 8);

    validate(vector.data_type() == DataType::Int64, "vector data type");
    validate(vector.encoding() == VectorEncoding::Flat, "vector encoding");

    validate(vector.size() == 0, "vector initial size");
    validate(vector.capacity() == 8, "vector capacity");

    validate(vector.is_writable(), "vector is writable");
    std::cout << "[PASS] test_flat_vector_metadata" << std::endl;
}


void test_flat_vector_write_and_read()
{
    auto vector = FlatVector::create(DataType::Int64, 8);
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
    auto vector = FlatVector::create(DataType::Float64, 4);
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

void test_flat_vector_external_raed_access()
{
    auto owner = std::make_shared<std::vector<std::int64_t>>(
        std::initializer_list<std::int64_t>{10, 20, 30});

    auto vector = FlatVector::wrap_external(volap::core::DataType::Int64,
                                            owner->data(),
                                            owner->size(),
                                            owner);

    validate(vector.size() == 3, "external size");

    validate(
        vector.capacity() == 3,
        "external capacity"
    );

    validate(
        !vector.is_writable(),
        "external vector is read-only"
    );

    const auto *values = FlatVector::get_data<std::int64_t>(vector);

    validate(values[0] == 10, "external value 0");
    validate(values[1] == 20, "external value 1");
    validate(values[2] == 30, "external value 2");

    validate_throws<std::logic_error>(
        [&] {
            (void)FlatVector::get_mutable_data<std::int64_t>(vector);
        },
        "external vector immutable"
    );

    std::cout << "[PASS] test_flat_vector_external_raed_access" << std::endl;
}

void test_flat_vector_external_lifetime()
{
    std::weak_ptr<std::vector<std::int64_t>> weak_owner;
    {
        auto owner = std::make_shared<std::vector<std::int64_t>>(
            std::initializer_list<std::int64_t>{10, 20, 30});

        weak_owner = owner;
        auto vector = FlatVector::wrap_external(DataType::Int64,
                                                owner->data(),
                                                owner->size(),
                                                owner);
        owner.reset();
        validate(!weak_owner.expired(),
                 "external owner pinned by Vector");

        validate(FlatVector::get_data<std::int64_t>(vector)[2] == 30,
                 "pinned external memory readable");
    }

    validate(weak_owner.expired(),
             "external owner released with Vector out of scope");
    std::cout << "[PASS] test_flat_vector_external_lifetime" << std::endl;
}

void test_flat_vector_reference_sharing_buffer()
{
    auto original = FlatVector::create(DataType::Int64, 4);

    auto *data = FlatVector::get_mutable_data<std::int64_t>(original);

    data[0] = 7;
    data[1] = 9;

    original.set_size(2);

    {
        auto alias = original.reference();

        validate(
            !original.is_writable(),
            "original is not writable while referenced"
        );

        validate(
            !alias.is_writable(),
            "alias is not writable"
        );

        const auto *alias_data = FlatVector::get_data<std::int64_t>(alias);

        validate(alias_data[0] == 7, "alias value 0");
        validate(alias_data[1] == 9, "alias value 1");

        validate_throws<std::logic_error>(
            [&] {
                (void)FlatVector::get_mutable_data<std::int64_t>(original);
            },
            "shared vector immutable"
        );
    }

    validate(
        original.is_writable(),
        "original writable after alias destruction"
    );

    auto *mutable_values = FlatVector::get_mutable_data<std::int64_t>(original);

    mutable_values[0] = 100;

    validate(
        FlatVector::get_data<std::int64_t>(original)[0] == 100,
        "unique vector mutable again"
    );

    std::cout << "[PASS] test_flat_vector_reference_sharing_buffer" << std::endl;
}

} // namespace


int main() 
{
    test_flat_vector_metadata();
    test_flat_vector_write_and_read();
    test_flat_vector_clear_reuses_allocation();
    test_flat_vector_external_raed_access();
    test_flat_vector_external_lifetime();
    test_flat_vector_reference_sharing_buffer();
}
