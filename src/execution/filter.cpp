
#include "volap/execution/filter.h"
#include "volap/kernels/select.h"

namespace volap::execution 
{

using namespace volap::kernels;

namespace 
{


} // anonymous namespace


Filter::Filter(std::size_t column_index, Threshold threshold) 
    : column_index_(column_index), 
      threshold_(std::move(threshold))
{
}

Filter Filter::i64_greater_than(std::size_t column_index,
                                std::int64_t threshold)
{
    return Filter(column_index, threshold);
}


Filter Filter::f32_greater_than(std::size_t column_index,
                                float threshold)
{
    return Filter(column_index, threshold);
}

Filter Filter::f64_greater_than(std::size_t column_index,
                                double threshold
)
{
    return Filter(column_index, threshold);
}


void Filter::select(const DataChunk &input)
{
    if (column_index_ >= input.column_count()) {
        throw std::out_of_range(
            "Filter: predicate column index is out of range");
    }

    const Vector &predicate_column = input.column(column_index_);
    using namespace volap::kernels;

    std::visit(
        [&](const auto threshold) {
            using ThresholdType = std::decay_t<decltype(threshold)>;

            if constexpr (std::is_same_v<ThresholdType, std::int64_t>) 
            {
                select_i64_gt(predicate_column, threshold, selection_);
            } 
            else if constexpr (std::is_same_v<ThresholdType, float>) 
            {
                select_f32_gt(predicate_column, threshold, selection_);
            } 
            else if constexpr (std::is_same_v<ThresholdType, double>) 
            {
                select_f64_gt(predicate_column, threshold, selection_);
            }
        },
        threshold_
    );
}

void Filter::execute(const DataChunk &input,
                     DataChunk &output)
{
    if (&input == &output) {
        throw std::invalid_argument(
            "Filter: in-place filtering is not supported");
    }

    select(input);
}

} // volap::execution
