#pragma once

#include <cstdint>
#include <type_traits>

namespace volap::core {

enum class DataType 
{
    Bool8,
    Int64,
    Float32,
    Float64
    // TO DO add more complex types 
};


inline constexpr std::size_t data_type_size(DataType type) noexcept
{
    switch (type) {
        case DataType::Bool8:
            // vector of std::bool won't work due to compression
            return sizeof(std::uint8_t);

        case DataType::Int64:
            return sizeof(std::int64_t);

        case DataType::Float32:
            return sizeof(float);

        case DataType::Float64:
            return sizeof(double);
    }
    return 0;
}


// Zero-overhead translation from compile-time type to runtime enum
template <typename T>
struct DataTypeOf;

template <>
struct DataTypeOf<std::uint8_t> {
    static constexpr DataType value = DataType::Bool8;
};

template <>
struct DataTypeOf<std::int64_t> {
    static constexpr DataType value = DataType::Int64;
};

template <>
struct DataTypeOf<float> {
    static constexpr DataType value = DataType::Float32;
};

template <>
struct DataTypeOf<double> {
    static constexpr DataType value = DataType::Float64;
};


// Avoid fetching from memory, bake the type enum into instruction.
// Remove const and volatile
template <typename T>
inline constexpr DataType data_type_v = DataTypeOf<std::remove_cv_t<T>>::value;


} // namespace volap::core
