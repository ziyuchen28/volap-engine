#pragma once

#include <cstdint>
#include <type_traits>

namespace volap::core {

enum class Type 
{
    Bool8,
    Int64,
    Float32,
    Float64
    // TO DO add more complex types 
};


inline constexpr std::size_t type_size(Type type) noexcept
{
    switch (type) 
    {
        case Type::Bool8:
            return sizeof(std::uint8_t);
        case Type::Int64:
            return sizeof(std::int64_t);
        case Type::Float32:
            return sizeof(float);
        case Type::Float64:
            return sizeof(double);
    }
    return 0;
}


// Zero-overhead translation from compile-time type to runtime enum
template <typename T>
struct TypeOf;

template <>
struct TypeOf<std::uint8_t> {
    static constexpr Type value = Type::Bool8;
};

template <>
struct TypeOf<std::int64_t> {
    static constexpr Type value = Type::Int64;
};

template <>
struct TypeOf<float> {
    static constexpr Type value = Type::Float32;
};

template <>
struct TypeOf<double> {
    static constexpr Type value = Type::Float64;
};


// Avoid fetching from memory, bake the type enum into instruction.
// Remove const and volatile
template <typename T>
inline constexpr Type type_v = TypeOf<std::remove_cv_t<T>>::value;


} // namespace volap::core
