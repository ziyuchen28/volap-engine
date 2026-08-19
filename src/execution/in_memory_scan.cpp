
#include "volap/execution/in_memory_scan.h"
#include "volap/core/vector.h"
#include "volap/core/flat_vector.h"

#include <cstring>

namespace volap::execution 
{

using namespace volap::core;

namespace 
{

template <typename T>
void copy_flat_range(const Vector &source,
                     std::size_t source_offset,
                     Vector &destination,
                     std::size_t row_count)
{
    // We must avoid accidental shallow copying in memcpy
    static_assert(
        std::is_trivially_copyable_v<T>,
        "InMemoryScan supports only trivially copyable primitive types");

    if (row_count == 0) {
        return;
    }

    const T *source_data = FlatVector::get_data<T>(source);
    T *destination_data = FlatVector::get_mutable_data<T>(destination);

    std::memcpy(destination_data,
                source_data + source_offset,
                row_count * sizeof(T));
}

void copy_range(const Vector &src,
                std::size_t src_offset,
                Vector &dest,
                std::size_t row_count)
{
    switch (src.data_type()) {
        case Type::Bool8:
            copy_flat_range<std::uint8_t>(src, src_offset, dest, row_count);
            return;

        case Type::Int64:
            copy_flat_range<std::int64_t>(src, src_offset, dest, row_count);
            return;

        case Type::Float32:
            copy_flat_range<float>(src, src_offset, dest, row_count);
            return;

        case Type::Float64:
            copy_flat_range<double>(src, src_offset, dest, row_count);
            return;
    }

    throw std::logic_error(
        "InMemoryScan: unsupported source Type"
    );
}

} // anynomous namespace

InMemoryScan::InMemoryScan(DataChunk source, std::size_t chunk_size)
    : source_(std::move(source)),
      chunk_size_(chunk_size)
{
    if (chunk_size_ == 0) {
        throw std::invalid_argument(
            "InMemoryScan: chunk size must be greater than zero");
    }
}

// On first call, allocate reusable output buffers and create corresponding columns.
// On subsequent calls, reuse the same output buffer 
void InMemoryScan::prepare_output(DataChunk &output) const
{
    // First call, allocate reusable buffers and initialzie columns
    if (output.column_count() == 0) {
        output.reserve_columns(source_.column_count());

        for (std::size_t column_index = 0;
             column_index < source_.column_count();
             ++column_index) 
        {
            const Vector &source_column = source_.column(column_index);
            if (source_column.encoding() != VectorEncoding::Flat) 
            // TO DO support other encoding
            {
                throw std::logic_error(
                    "InMemoryScan: currently supports only Flat vector");
            }

            output.add_column(
                FlatVector::create(source_column.data_type(), chunk_size_));
        }

        return;
    }

    // On subsequent calls, verify output buffer compatability.
    // in case caller passed incorrect buffer
    if (output.column_count() != source_.column_count()) {
        throw std::invalid_argument(
            "InMemoryScan: invalid output buffer, size mismatch");
    }

    // TO DO, let scan operator takes ownship of the output buffer instead of 
    // caller owning it to avoid repearted check? 
    // (current trade-off is to follow more natural ownership, 
    // more validation needed from operator side, 
    // although it's not in hot path since this is called in batch)
    for (std::size_t column_index = 0;
         column_index < source_.column_count();
         ++column_index)
    {
        const Vector &source_column = source_.column(column_index);
        Vector &output_column = output.column(column_index);

        if (source_column.data_type() != output_column.data_type()) {
            throw std::invalid_argument(
                "InMemoryScan: output column Type does not match source");
        }

        if (source_column.encoding() != volap::core::VectorEncoding::Flat 
            || output_column.encoding() != volap::core::VectorEncoding::Flat) 
        {
            throw std::logic_error(
                "InMemoryScan: currently supports only Flat vectors");
        }

        if (output_column.capacity() < chunk_size_) {
            throw std::invalid_argument(
                "InMemoryScan: output column capacity is smaller "
                "than the configured chunk size");
        }

        if (!output_column.is_writable()) {
            throw std::logic_error(
                "InMemoryScan: output column buffer is not writable");
        }
    }

    // clear the previous count
    output.clear();
}

bool InMemoryScan::next(volap::core::DataChunk &output)
{
    if (position_ >= source_.row_count()) {
        output.clear();
        return false;
    }

    prepare_output(output);

    const std::size_t remaining_rows = source_.row_count() - position_;
    const std::size_t rows_to_emit = std::min(chunk_size_, remaining_rows);

    for (std::size_t column_index = 0;
         column_index < source_.column_count();
         ++column_index) 
    {
        copy_range(source_.column(column_index),
                   position_,
                   output.column(column_index),
                   rows_to_emit);
    }

    output.set_row_count(rows_to_emit);
    position_ += rows_to_emit;

    return true;
}


} // namespace volap::execution



