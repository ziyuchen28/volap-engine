
#include <stdexcept>
#include <cstring>

#include "volap/execution/project.h"
#include "volap/core/flat_vector.h"

namespace volap::execution
{

namespace 
{

Type checked_projection_type(const Projection &projection,
                             const DataChunk &input)
{
    switch (projection.type()) {
        case ProjectionType::ColumnRef: {
            return input.column(projection.left_column_index()).data_type();
        }

        case ProjectionType::Multiply: {
            const Vector &left = input.column(projection.left_column_index());
            const Vector &right = input.column(projection.right_column_index());

            // TO DO: handle implicit conversion, for example int64 * float32/64
            if (left.data_type() != right.data_type()) {
                throw std::invalid_argument(
                    "Project: multiply input types do not match");
            }

            if (left.data_type() != Type::Float32 &&
                left.data_type() != Type::Float64) {
                throw std::invalid_argument(
                    "Project: multiply currently supports only f32/64");
            }

            return left.data_type();
        }
    }

    throw std::logic_error(
        "Project: unsupported projection");
}


std::size_t projection_capacity(const Projection &projection,
                                const DataChunk &input)
{
    switch (projection.type()) {
        case ProjectionType::ColumnRef: 
        {
            const Vector &column = input.column(projection.left_column_index());
            return column.capacity();
        }

        case ProjectionType::Multiply: 
        {
            const Vector &left = input.column(projection.left_column_index());
            const Vector &right = input.column(projection.right_column_index());
            return std::min(left.capacity(), right.capacity());
        }
    }

    throw std::logic_error(
        "Project: unsupported projection");
}


template <typename T>
void copy_flat_column(const Vector &input, Vector &output, std::size_t row_count)
{
    if (row_count == 0) {
        return;
    }

    const T *input_data = FlatVector::get_data<T>(input);
    T *output_buffer = FlatVector::get_mutable_data<T>(output);

    std::memcpy(output_buffer, input_data, row_count * sizeof(T));
}

void copy_column(const Vector &input, Vector &output, std::size_t row_count)
{
    switch (input.data_type()) 
    {
        case Type::Bool8:
            copy_flat_column<std::uint8_t>(input, output, row_count);
            return;

        case Type::Int64:
            copy_flat_column<std::int64_t>(input, output, row_count);
            return;

        case Type::Float32:
            copy_flat_column<float>(input, output, row_count);
            return;

        case Type::Float64:
            copy_flat_column<double>(input, output, row_count);
            return;
    }

    throw std::logic_error(
        "Project: unsupported input data type");
}

} // anonymous namespace



Project::Project(std::vector<Projection> projections)
    : projections_(std::move(projections))
{
    if (projections_.empty()) {
        throw std::invalid_argument(
            "Project: at least one projection is required");
    }
}

Project::Project(std::initializer_list<Projection> projections)
    : projections_(projections)
{
    if (projections_.empty()) {
        throw std::invalid_argument(
            "Project: at least one projection is required");
    }
}


void Project::prepare_output(const DataChunk &input,
                             DataChunk &output) const
{
    if (output.column_count() == 0) {
        output.reserve_columns(projections_.size());
        for (const Projection &projection : projections_) {
            const Type result_type = checked_projection_type(projection, input);
            const std::size_t capacity = projection_capacity(projection, input);
            output.add_column(FlatVector::create(result_type, capacity));
        }
        return;
    }

    if (output.column_count() != projections_.size()) {
        throw std::invalid_argument(
            "Project: output column count does not match projection count");
    }

    output.clear();
}

void Project::evaluate_projection(const Projection &projection,
                                  const DataChunk &input,
                                  Vector &output) const
{
    switch (projection.type()) 
    {
        case ProjectionType::ColumnRef: {
            const Vector &column = input.column(projection.left_column_index());
            copy_column(column, output, input.row_count());
            return;
        }
        case ProjectionType::Multiply: {
            const Vector &left = input.column(projection.left_column_index());
            const Vector &right = input.column(projection.right_column_index());
            // TO DO
            //multiply_columns(left, right, output, input.row_count());
            return;
        }
    }

    throw std::logic_error(
        "Project: unsupported projection");
}

void Project::execute(const DataChunk &input, DataChunk &output) const
{
    if (&input == &output) {
        throw std::invalid_argument(
            "Project: in-place execution is not supported");
    }

    prepare_output(input, output);

    for (std::size_t i = 0; i < projections_.size(); ++i) {
        evaluate_projection(projections_[i], input, output.column(i));
    }

    output.set_row_count(input.row_count());
}

} // namespace volap::execution

