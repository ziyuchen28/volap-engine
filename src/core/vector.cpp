#include "volap/core/vector.h"

#include <format>

namespace volap::core 
{

Vector Vector::reference() const noexcept
{
    // Makes a copy of the meta data while sharing the same underlying buffer
    return Vector(data_type_,
                  encoding_,
                  size_,
                  capacity_,
                  buffer_);
}


void Vector::validate_encoding(VectorEncoding requested_encoding) const
{
    if (encoding_ != requested_encoding) {
        throw std::logic_error(
            std::format("Vector encoding mismatch. Requested: {}, Expected: {}",
                        static_cast<int>(requested_encoding),
                        static_cast<int>(encoding_))
        );
    }
}


void Vector::validate_data_type(DataType requested_type) const
{
    if (data_type_ != requested_type) {
        throw std::logic_error(
            std::format("Vector data type mismatch. Requested: {}, Expected: {}",
                        static_cast<int>(requested_type),
                        static_cast<int>(data_type_))
        );
    }
}

void Vector::clear() noexcept
{
    size_ = 0;
}

[[noreturn]] void Vector::throw_capacity_error() const 
{
    throw std::invalid_argument(
        "row count exceeds maximum allocated capacity."
    );
}


} // namespace volap::core
