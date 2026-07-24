
#include "volap/core/flat_vector.h"

namespace volap::core
{


Vector FlatVector::create(Type type,
                          std::size_t row_count)
{
    const std::size_t bytes = checked_bytes_size(type, row_count);

    return Vector(type,
                  VectorEncoding::Flat,
                  0,
                  row_count,
                  Buffer::allocate(bytes));
}

Vector FlatVector::wrap_external(Type type,
                                 const void *data,
                                 std::size_t row_count,
                                 Buffer::BufferLifetime lifetime)
{
    if (row_count != 0 && !satisfies_natural_alignment(data, type)) {
        throw std::invalid_argument(
            "Flat Vector: data is not naturally aligned "
        );
    }

    const std::size_t bytes = checked_bytes_size(type, row_count);

    return Vector(type,
                  VectorEncoding::Flat,
                  row_count,
                  row_count,
                  Buffer::wrap_external(data,
                                        bytes,
                                        std::move(lifetime)));
}

void FlatVector::validate_access(const Vector &vector,
                                 Type requested_type)
{
    vector.validate_encoding(VectorEncoding::Flat);
    vector.validate_data_type(requested_type);
}


} // namespace volap::core
