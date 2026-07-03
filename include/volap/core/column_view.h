#pragma once

#include "volap/core/type.h"

#include <cstddef>
#include <stdexcept>

namespace volap::core {

// This represent a view over contiguous column data
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

    bool empty() const noexcept
    {
        return row_count_ == 0;
    }


private:
    Type type_ = Type::Int64;
    const void *data_ = nullptr;
    std::size_t row_count_ = 0;
};

} // namespace volap::core
