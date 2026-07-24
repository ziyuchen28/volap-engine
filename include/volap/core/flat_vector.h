
#pragma once

#include "volap/core/vector.h"

namespace volap::core 
{

// FlatVector is a stateless encoding helper
struct FlatVector final 
{
    FlatVector() = delete;

    // Creates a flat vector that owns the underlying storage
    static Vector create(Type type,
                         std::size_t row_capacity);

    // Creates a read-only flat Vector over external storage
    // data could come from:
    //   Arrow buffer
    //   Parquet batches
    //   mmap files
    //   std::vector
    //   ...
    static Vector wrap_external(Type type,
                                const void *data,
                                std::size_t row_count,
                                Buffer::BufferLifetime lifetime);

    // Read-only access to buffer data.
    template <typename T>
    static const T *get_data(const Vector &vector)
    {
        static_assert(std::is_same_v<T, std::remove_cv_t<T>>,
                     "T must not be const or volatile");

        validate_access(vector, type_v<T>);

        return reinterpret_cast<const T*>(vector.buffer_.data());
    }

    // Mutable access to the buffer data.
    template <typename T>
    static T *get_mutable_data(Vector &vector)
    {
        static_assert(std::is_same_v<T, std::remove_cv_t<T>>,
                     "T must not be const or volatile");

        validate_access(vector, type_v<T>);

        // byte* to T*
        return reinterpret_cast<T*>(vector.buffer_.mut_data());
    }


private:

    static void validate_access(const Vector &vector,
                                Type expected_type);


};

} // namespace volap::core 
