
#pragma once

#include "volap/core/data_chunk.h"
#include "volap/core/selection_vector.h"

#include <cstdint>
#include <variant>

namespace volap::execution 
{

using namespace volap::core;

class Filter final {

public:
    static Filter i64_greater_than(std::size_t column_index,
                                   std::int64_t threshold);

    static Filter f32_greater_than(std::size_t column_index,
                                   float threshold);

    static Filter f64_greater_than(std::size_t column_index,
                                   double threshold);

    // Applies the predicate to one input batch and materializes all
    // selected rows into output.
    void execute(const DataChunk &input, DataChunk &output);

    const SelectionVector &selection() const noexcept
    {
        return selection_;
    }

private:

    using Threshold = std::variant<std::int64_t, float, double>;

    Filter(std::size_t column_index, Threshold threshold);

    // This invokes the select kernel and produces the logical input
    // row indexes that satisfy the predicate.
    void select_input(const DataChunk &input);

    // Allocate output vectors on the first call, 
    // or reuse their buffers on subsequent calls.
    void prepare_output(const DataChunk &input, DataChunk &output) const;

    std::size_t column_id_;
    Threshold threshold_;
    SelectionVector selection_;

};


} // namespace volap::execution
