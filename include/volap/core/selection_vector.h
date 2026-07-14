#pragma once

#include <vector>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <span>

namespace volap::core {

constexpr auto U32_MAX = std::numeric_limits<std::uint32_t>::max();

class SelectionVector 
{
public:
    SelectionVector() = default;

    explicit SelectionVector(std::size_t count)
    {
        indices_.reserve(count);
    }

    void reserve(std::size_t count)
    {
        indices_.reserve(count);
    }

    void clear() noexcept
    {
        indices_.clear();
    }

    bool empty() const noexcept
    {
        return indices_.empty();
    }

    // This is usually called while looping through a DataChunk, 
    // hence expecting a size_t
    void push_back(std::size_t row_index)
    {
        if (row_index > U32_MAX) {
            throw std::out_of_range("SelectionVector: row index exceeds uint32_t");
        }

        indices_.push_back(static_cast<std::uint32_t>(row_index));
    }

    std::size_t size() const noexcept
    {
        return indices_.size();
    }

    std::size_t capacity() const noexcept
    {
        return indices_.capacity();
    }

    std::uint32_t operator[](std::size_t index) const noexcept
    {
        return indices_[index];
    }

    const std::uint32_t *data() const noexcept
    {
        return indices_.data();
    }

    std::span<const std::uint32_t> as_span() const noexcept
    {
        return std::span<const std::uint32_t>(indices_.data(), indices_.size());
    }

private:
    // Relative position in the batch not absolute
    // Prefer uint32 over uint16 for flexibility
    // Does not need to be 64 bits as it's a batch of the data not the full data
    std::vector<std::uint32_t> indices_;
};

} // namespace volap::core
