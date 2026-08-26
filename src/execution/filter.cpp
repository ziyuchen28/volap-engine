
#include "volap/execution/filter.h"
#include "volap/kernels/select.h"
#include "volap/core/flat_vector.h"

namespace volap::execution 
{

using namespace volap::kernels;

namespace 
{

template <typename T>
void copy_selected_rows(const Vector &input,
                        const SelectionVector &selection,
                        Vector &output)
{
    const T *input_data = FlatVector::get_data<T>(input);
    T *output_buffer = FlatVector::get_mutable_data<T>(output);

    // Ouput vector size should match selection vector size
    for (std::size_t output_row = 0;
         output_row < selection.size();
         ++output_row) 
    {
        const std::size_t input_row = selection[output_row];
        output_buffer[output_row] = input_data[input_row];
    }
}

void copy_selected_column(const Vector &input,
                          const SelectionVector &selection,
                          Vector &output)
{
    switch (input.data_type()) {
        case Type::Bool8:
            copy_selected_rows<std::uint8_t>(input, selection, output);
            return;

        case Type::Int64:
            copy_selected_rows<std::int64_t>(input, selection, output);
            return;

        case Type::Float32:
            copy_selected_rows<float>(input, selection, output);
            return;

        case Type::Float64:
            copy_selected_rows<double>(input, selection, output);
            return;
    }
}

} // anonymous namespace


Filter::Filter(std::size_t column_index, Threshold threshold) 
             : column_id_(column_index), 
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


void Filter::select_input(const DataChunk &input)
{
    if (column_id_ >= input.column_count()) {
        throw std::out_of_range(
            "Filter: predicate column index is out of range");
    }

    const Vector &predicate_column = input.column(column_id_);
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

void Filter::prepare_output(const DataChunk &input, DataChunk &output) const
{
    if (output.column_count() == 0) {
        output.reserve_columns(input.column_count());

        for (std::size_t column_index = 0;
             column_index < input.column_count();
             ++column_index) 
        {
            const Vector &input_column = input.column(column_index);

            if (input_column.encoding() != VectorEncoding::Flat) {
                throw std::logic_error(
                    "Filter: currently supports only Flat vectors");
            }

            // Allocate based on the worst case upfront and resue buffer 
            // for subsequent selections, instead of allocate on demand 
            // based on the slection size each time.
            output.add_column(
                FlatVector::create(input_column.data_type(), 
                                   input.row_count()));
        }

        return;
    }

    if (output.column_count() != input.column_count()) {
        throw std::invalid_argument(
            "Filter: output column count does not match input");
    }

    // Basic validations before clearing the previous batch.
    for (std::size_t column_index = 0;
         column_index < input.column_count();
         ++column_index) 
    {
        const Vector &input_column = input.column(column_index);
        Vector &output_column = output.column(column_index);

        if (input_column.data_type() != output_column.data_type()) {
            throw std::invalid_argument(
                "Filter: output column type does not match input");
        }

        if (input_column.encoding() != VectorEncoding::Flat 
            || output_column.encoding() != VectorEncoding::Flat) 
        {
            throw std::logic_error(
                "Filter currently supports only Flat vectors");
        }

        if (output_column.capacity() < input.row_count()) 
        {
            throw std::invalid_argument(
                "Filter: output column capacity is too small"
            );
        }
        if (!output_column.is_writable()) 
        {
            throw std::logic_error(
                "Filter: output column is not writable");
        }
    }

    // Reusing the allocations from last batch.
    output.clear();
}

void Filter::execute(const DataChunk &input,
                     DataChunk &output)
{
    if (&input == &output) {
        throw std::invalid_argument(
            "Filter: in-place filtering is not supported");
    }

    select_input(input);

    prepare_output(input, output);

    // TO DO: idempotency?
    // avoid redundant copying when execute invoked the second time? 
    for (std::size_t column_index = 0;
         column_index < input.column_count();
         ++column_index) 
    {
        copy_selected_column(input.column(column_index),
                             selection_,
                             output.column(column_index));
    }

    output.set_row_count(selection_.size());
}



} // volap::execution
