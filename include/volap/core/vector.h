#pragma once

#include "volap/core/buffer.h"
#include "volap/core/type.h"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <limits>

namespace volap::core {


inline std::size_t checked_bytes_size(Type type, std::size_t row_count)
{
    const std::size_t size = type_size(type);

    if (size == 0) {
        throw std::invalid_argument("Vector data type has zero physical width");
    }
    if (row_count > std::numeric_limits<std::size_t>::max() / size) {
        throw std::invalid_argument("Vector size overflow");
    }

    return row_count * size;
}

inline std::size_t natural_alignment(Type type) noexcept
{
    switch (type) 
    {
        case Type::Bool8:
            return alignof(std::uint8_t);

        case Type::Int64:
            return alignof(std::int64_t);

        case Type::Float32:
            return alignof(float);

        case Type::Float64:
            return alignof(double);
    }
    return 1;
}

inline bool satisfies_natural_alignment(const void *data, Type type) noexcept
{
    if (data == nullptr) {
        return true;
    }

    const auto address = reinterpret_cast<std::uintptr_t>(data);

    return address % natural_alignment(type) == 0;
}


enum class VectorEncoding {
    Flat
};

class Vector {
public:
    ~Vector() = default;

    // A normal copy would share the underlying buffer
    // See reference() for shallow copy
    // TO DO: provide a deep copy instead 
    Vector(const Vector&) = delete;
    Vector& operator=(const Vector&) noexcept = default;

    Vector(Vector&&) noexcept = default;
    Vector& operator=(Vector&&) noexcept = default;

    Type data_type() const noexcept
    {
        return data_type_;
    }

    VectorEncoding encoding() const noexcept
    {
        return encoding_;
    }

    // Current size
    std::size_t size() const noexcept
    {
        return size_;
    }

    // temp w/a
    // TO DO: overload [] to adjust size automatically?
    void set_size(std::size_t row_count)
    {
        if (row_count > capacity_) {
            // set_size could be within a hot path,
            // prefer to move throw statement out of header to avoid injecting 
            // the assembly code associated with it.
            throw_capacity_error();
        }
        size_ = row_count;
    }

    // Max size bound by allocation
    std::size_t capacity() const noexcept
    {
        return capacity_;
    }

    std::size_t size_bytes() const noexcept
    {
        return size_ * type_size(data_type_);
    }

    bool empty() const noexcept
    {
        return size_ == 0;
    }

    bool is_writable() noexcept
    {
        return buffer_.is_writable();
    }

    Vector reference() const noexcept;

    void clear() noexcept;

private:

    friend struct FlatVector;

    Vector(Type type, 
           VectorEncoding encoding,
           std::size_t size, 
           std::size_t capacity, 
           Buffer buffer) noexcept 
        : data_type_(type),
          encoding_(encoding),
          size_(size),
          capacity_(capacity),
          buffer_(std::move(buffer))
    {}

    void validate_encoding(VectorEncoding requested_encoding) const;

    void validate_data_type(Type requested_type) const;

    Type data_type_;
    VectorEncoding encoding_;

    std::size_t size_;
    std::size_t capacity_;

    Buffer buffer_;

    [[noreturn]] void throw_capacity_error() const;   
};

} // namespace volap::core
