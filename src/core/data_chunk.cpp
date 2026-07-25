
#include "volap/core/data_chunk.h"

namespace volap::core
{

    
DataChunk::DataChunk(DataChunk &&other) noexcept
    : columns_(std::move(other.columns_)),
      row_count_(std::exchange(other.row_count_, 0))
{}

DataChunk &DataChunk::operator=(DataChunk &&other) noexcept
{
    if (this != &other) {
        columns_ = std::move(other.columns_);
        row_count_ = std::exchange(other.row_count_, 0);
    }
    return *this;
}

void DataChunk::add_column(Vector &&vector)
{
    std::size_t vec_size = vector.size();

    bool first_column = columns_.empty();

    if (!first_column && vec_size != row_count_) {
        // One data chunk is a stripe across a portion of a table 
        throw std::invalid_argument(
                "DataChunk: Vector size and DataChunk row count mismatch");
    }

    columns_.push_back(std::move(vector));

    if (first_column) {
        row_count_ = vec_size;
    }
}

void DataChunk::reserve_columns(std::size_t column_count)
{
   columns_.reserve(column_count); 
}


const Vector &DataChunk::column(std::size_t index) const
{
    if (index >= columns_.size()) {
        throw std::out_of_range("DataChunk: column");
    }
    return columns_[index];
}

Vector &DataChunk::column(std::size_t index) 
{
    if (index >= columns_.size()) {
        throw std::out_of_range("DataChunk: column");
    }
    return columns_[index];
}


void DataChunk::set_row_count(std::size_t row_count)
{
    // Validate every column before modifying any of them.
    // This avoids partially updating the chunk if one column does not
    // have enough capacity.
    for (const Vector &vector : columns_) {
        if (row_count > vector.capacity()) {
            throw std::out_of_range(
                "DataChunk: row count exceeds column capacity");
        }
    }

    for (Vector &vector : columns_) {
        vector.set_size(row_count);
    }


    row_count_ = row_count;
}


void DataChunk::clear() noexcept
{
    for (Vector& vector : columns_) {
        vector.clear();
    }

    row_count_ = 0;
}

} // namespace volap::core
