#pragma once

#include "volap/core/type.h"

#include <cstddef>
#include <stdexcept>
#include <span>

namespace volap::core {

// Type erased view over contiguous column data, effectively type erased std::span,
// this allows engine pass around columes generically.
class ColumnView 
{
public:
    ColumnView() = default;

    ColumnView(Type type, const void *data, std::size_t row_count)
        : type_(type),
          data_(data),
          row_count_(row_count)
    {
        if (row_count_ > 0 && data_ == nullptr) {
            throw std::invalid_argument("ColumnView: non-empty column has null data");
        }
    }

    template <typename T>
    ColumnView(const T *data, std::size_t row_count)
        : ColumnView(type_v<T>, data, row_count)
    {}

    Type type() const noexcept
    {
        return type_;
    }

    const void *raw_data() const noexcept
    {
        return data_;
    }

    std::size_t row_count() const noexcept
    {
        return row_count_;
    }
    
    std::size_t byte_size() const noexcept
    {
        return row_count_ * type_size(type_);
    }

    bool empty() const noexcept
    {
        return row_count_ == 0;
    }


    template <typename T>
    // const T: Returning a read-only span
    std::span<const T> as_span() const
    {
        using STORED_TYPE = std::remove_cv_t<T>;
        if (type_ != type_v<STORED_TYPE>) {
            throw std::logic_error("ColumnView::as_span: type mismatch");
        }
        return std::span<const T>(static_cast<const T*>(data_), row_count_);
    }


private:
    Type type_ = Type::Int64;
    const void *data_ = nullptr;
    std::size_t row_count_ = 0;
};

} // namespace volap::core
