
#pragma once

#include "volap/core/column_view.h"

#include <vector>

namespace volap::core {

// This owns a list of views of columns
class DataChunk 
{
public:
    DataChunk() = default;

    DataChunk(std::initializer_list<ColumnView> columns)
    {
        for (const ColumnView &column : columns) {
            add_column(column);
        }
    }

    explicit DataChunk(std::vector<ColumnView> columns)
    {
        for (const ColumnView &column : columns) {
            add_column(column);
        }
    }

    void add_column(ColumnView column)
    {
        if (columns_.empty()) {
            row_count_ = column.row_count();
        } else if (column.row_count() != row_count_) {
            throw std::invalid_argument("DataChunk: column row count mismatch");
        }

        columns_.push_back(column);
    }

    const ColumnView &column(std::size_t index) const
    {
        if (index >= columns_.size()) {
            throw std::out_of_range("DataChunk::column");
        }
        return columns_[index];
    }

    std::size_t column_count() const noexcept
    {
        return columns_.size();
    }

    std::size_t row_count() const noexcept
    {
        return row_count_;
    }

    bool empty() const noexcept
    {
        return row_count_ == 0;
    }

    const std::vector<ColumnView> &columns() const noexcept
    {
        return columns_;
    }

private:
    std::vector<ColumnView> columns_;
    std::size_t row_count_ = 0;
};

} // namespace volap::core
