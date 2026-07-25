
#pragma once

#include "volap/core/vector.h"

#include <vector>

namespace volap::core {

// This owns a list of views of columns
class DataChunk final
{
public:

    DataChunk() noexcept = default;
    ~DataChunk() = default;

    DataChunk(const DataChunk&) = delete;
    DataChunk& operator=(const DataChunk&) = delete;

    DataChunk(DataChunk &&other) noexcept;
    DataChunk& operator=(DataChunk &&other) noexcept;

    void reserve_columns(std::size_t column_count);

    // Transfers ownership of the Vector object into this chunk.
    // The Vector's logical size must match the current chunk row count.
    // For the first column, its size establishes the initial row count.
    void add_column(Vector &&vector);

    const Vector &column(std::size_t index) const;

    // For getting mutable column
    Vector &column(std::size_t index);

    std::size_t column_count() const noexcept
    {
        return columns_.size();
    }

    std::size_t row_count() const noexcept
    {
        return row_count_;
    }
    
    // This also sets the vector size for each column
    // This is mainly used in batch operation hot path to avoid incrementing size 
    // for each element added.
    void set_row_count(std::size_t row_count);

    // Fast reset:
    // Only reset the size, internal memory not touched
    void clear() noexcept;

    bool empty() const noexcept
    {
        return row_count_ == 0;
    }

private:
    std::vector<Vector> columns_;
    std::size_t row_count_ = 0;
};

} // namespace volap::core
