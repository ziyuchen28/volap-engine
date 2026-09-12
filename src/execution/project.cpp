// TO DO
//  columnRef is actually ref, no copy


#include <stdexcept>
#include <cstring>

#include "volap/execution/project.h"

#include "volap/kernels/multiply.h"
#include "volap/kernels/cast.h"

#include "volap/core/flat_vector.h"

namespace volap::execution
{

namespace 
{

using namespace volap::kernels;

// Type checked_projection_type(const Projection &projection,
//                              const DataChunk &input)
// {
//     switch (projection.type()) {
//         case ProjectionType::ColumnRef: {
//             return input.column(projection.left_column_index()).data_type();
//         }
//
//         case ProjectionType::Multiply: {
//             const Vector &left = input.column(projection.left_column_index());
//             const Vector &right = input.column(projection.right_column_index());
//
//             // TO DO: handle implicit conversion, for example int64 * float32/64
//             if (left.data_type() != right.data_type()) {
//                 throw std::invalid_argument(
//                     "Project: multiply input types do not match");
//             }
//
//             if (left.data_type() != Type::Float32 &&
//                 left.data_type() != Type::Float64) {
//                 throw std::invalid_argument(
//                     "Project: multiply currently supports only f32/64");
//             }
//
//             return left.data_type();
//         }
//     }
//
//     throw std::logic_error(
//         "Project: unsupported projection");
// }
//

// std::size_t projection_capacity(const Projection &projection,
//                                 const DataChunk &input)
// {
//     switch (projection.type()) {
//         case ProjectionType::ColumnRef: 
//         {
//             const Vector &column = input.column(projection.left_column_index());
//             return column.capacity();
//         }
//
//         case ProjectionType::Multiply: 
//         {
//             const Vector &left = input.column(projection.left_column_index());
//             const Vector &right = input.column(projection.right_column_index());
//             return std::min(left.capacity(), right.capacity());
//         }
//     }
//
//     throw std::logic_error(
//         "Project: unsupported projection");
// }
//

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

void copy_column(Type data_type, const Vector &input, Vector &output, std::size_t row_count)
{
    switch (data_type) 
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

// // todo: mixed operand types
// void multiply_columns(const Vector &left,
//                       const Vector &right,
//                       Vector &output,
//                       std::size_t row_count)
// {
//     if (left.data_type() != right.data_type() ||
//         left.data_type() != output.data_type()) {
//         throw std::invalid_argument(
//             "Project: multiply vector types do not match");
//     }
//
//     switch (left.data_type()) 
//     {
//         case Type::Float32: 
//         {
//             const float *left_data = FlatVector::get_data<float>(left);
//             const float *right_data = FlatVector::get_data<float>(right);
//             float *output_data = FlatVector::get_mutable_data<float>(output);
//
//             multiply_f32_scalar(left_data, right_data, output_data, row_count);
//
//             return;
//         }
//         case Type::Float64: 
//         {
//             const double *left_data = FlatVector::get_data<double>(left);
//             const double *right_data = FlatVector::get_data<double>(right);
//             double *output_data = FlatVector::get_mutable_data<double>(output);
//
//             multiply_f64_scalar(left_data, right_data, output_data, row_count);
//
//             return;
//         }
//         // TO DO
//         case Type::Bool8:
//         case Type::Int64:
//             break;
//     }
//
//     throw std::invalid_argument(
//         "Project: multiply currently supports only "
//         "Float32 and Float64");
// }

} // anonymous namespace


Project::Project(std::vector<BoundProjection> projections)
    : projections_(std::move(projections)),
      scratch_(projections_.size())
{
    if (projections_.empty()) {
        throw std::invalid_argument(
            "Project: at least one projection is required");
    }
}

Project::Project(std::initializer_list<BoundProjection> projections)
    : projections_(projections),
      scratch_(projections_.size())
{
    if (projections_.empty()) {
        throw std::invalid_argument(
            "Project: at least one projection is required");
    }
}

Project::Project(std::vector<Projection> projections,
                 const DataSchema &input_schema)
    : projections_(bind_projections(projections, input_schema)),
      scratch_(projections_.size())
{
    if (projections_.empty()) {
        throw std::invalid_argument(
            "Project: at least one projection is required");
    }
}


// void Project::prepare_output(const DataChunk &input,
//                              DataChunk &output) const
// {
//     if (output.column_count() == 0) {
//         output.reserve_columns(projections_.size());
//         for (const Projection &projection : projections_) {
//             const Type result_type = checked_projection_type(projection, input);
//             const std::size_t capacity = projection_capacity(projection, input);
//             output.add_column(FlatVector::create(result_type, capacity));
//         }
//         return;
//     }
//
//     if (output.column_count() != projections_.size()) {
//         throw std::invalid_argument(
//             "Project: output column count does not match projection count");
//     }
//
//     output.clear();
// }

std::size_t projection_capacity(const BoundProjection &projection,
                                const DataChunk &input)
{
    switch (projection.type) {
        case BoundProjectionType::ColumnRef:
            return input.column(projection.left.column_index).capacity();

        case BoundProjectionType::MultiplyFloat32:
        case BoundProjectionType::MultiplyFloat64: {
            const Vector &left =
                input.column(projection.left.column_index);

            const Vector &right =
                input.column(projection.right.column_index);

            return std::min(left.capacity(), right.capacity());
        }
    }

    throw std::logic_error("Project: invalid bound projection type");
}

// Transform data based on cast types if needed and store in scratch buffer
const Vector &Project::prepare_operand(const BoundOperand &operand,
                                       std::optional<Vector> &scratch,
                                       const DataChunk &input)
{
    const Vector &source = input.column(operand.column_index);

    if (operand.cast == BoundCastType::None) {
        return source;
    }

    if (!scratch.has_value() || scratch->capacity() < source.capacity())
    {
        scratch = FlatVector::create(operand.execution_type, 
                                     source.capacity());
    }

    const std::size_t row_count = input.row_count();

    switch (operand.cast) {
        case BoundCastType::None:
            break;

        case BoundCastType::Int64ToFloat32: {
            const std::int64_t *source_data = FlatVector::get_data<std::int64_t>(source);
            float *scratch_data = FlatVector::get_mutable_data<float>(*scratch);
            volap::kernels::cast_i64_to_f32_scalar(source_data,
                                                   scratch_data,
                                                   row_count);
            break;
        }

        case BoundCastType::Int64ToFloat64: {
            const std::int64_t *source_data = FlatVector::get_data<std::int64_t>(source);
            double *scratch_data = FlatVector::get_mutable_data<double>(*scratch);
            volap::kernels::cast_i64_to_f64_scalar(source_data,
                                                   scratch_data,
                                                   row_count);
            break;
        }

        case BoundCastType::Float32ToFloat64: {
            const float *source_data = FlatVector::get_data<float>(source);
            double *scratch_data = FlatVector::get_mutable_data<double>(*scratch);
            volap::kernels::cast_f32_to_f64_scalar(source_data,
                                                   scratch_data,
                                                   row_count);
            break;
        }
    }

    scratch->set_size(row_count);

    return *scratch;
}

void Project::prepare_output(const DataChunk &input,
                             DataChunk &output) const
{
    if (output.column_count() == 0) {
        output.reserve_columns(projections_.size());

        for (const BoundProjection &projection : projections_) {
            const std::size_t capacity = projection_capacity(projection, input);
            output.add_column(FlatVector::create(projection.execution_type, capacity));
        }
        return;
    }

    if (output.column_count() != projections_.size()) {
        throw std::invalid_argument(
            "Project: output column count does not match projection count");
    }

    output.clear();
}

void Project::execute_projection(const BoundProjection &projection,
                                 ProjectionScratch &scratch,
                                 const DataChunk &input,
                                 Vector &output)
{
    const std::size_t row_count = input.row_count();

    switch (projection.type) {
        case BoundProjectionType::ColumnRef: {
            const Vector &source = input.column(projection.left.column_index);
            copy_column(projection.execution_type, source, output, row_count);
            return;
        }

        case BoundProjectionType::MultiplyFloat32: {
            const Vector &left = prepare_operand(projection.left, scratch.left, input);
            const Vector &right = prepare_operand(projection.right, scratch.right, input);

            const float *left_data = FlatVector::get_data<float>(left);
            const float *right_data = FlatVector::get_data<float>(right);
            float *output_data = FlatVector::get_mutable_data<float>(output);

            multiply_f32_scalar(left_data, 
                                right_data, 
                                output_data, 
                                row_count);
            return;
        }

        case BoundProjectionType::MultiplyFloat64: {
            const Vector &left = prepare_operand(projection.left, scratch.left, input);
            const Vector &right = prepare_operand(projection.right, scratch.right, input);

            const double *left_data = FlatVector::get_data<double>(left);
            const double *right_data = FlatVector::get_data<double>(right);

            double *output_data = FlatVector::get_mutable_data<double>(output);

            volap::kernels::multiply_f64_scalar(left_data,
                                                right_data,
                                                output_data,
                                                row_count);
            return;
        }
    }

    throw std::logic_error(
        "Project: invalid bound projection type");
}

// void Project::project_output(const Projection &projection,
//                                   const DataChunk &input,
//                                   Vector &output) const
// {
//     switch (projection.type()) 
//     {
//         case ProjectionType::ColumnRef: {
//             const Vector &column = input.column(projection.left_column_index());
//             copy_column(column, output, input.row_count());
//             return;
//         }
//         case ProjectionType::Multiply: {
//             const Vector &left = input.column(projection.left_column_index());
//             const Vector &right = input.column(projection.right_column_index());
//             multiply_columns(left, right, output, input.row_count());
//             return;
//         }
//     }
//
//     throw std::logic_error(
//         "Project: unsupported projection");
// }

void Project::execute(const DataChunk &input, DataChunk &output)
{
    if (&input == &output) {
        throw std::invalid_argument(
            "Project: in-place execution is not supported");
    }

    prepare_output(input, output);

    for (std::size_t i = 0; i < projections_.size(); ++i) {
        execute_projection(projections_[i], scratch_[i], input, output.column(i));
    }

    output.set_row_count(input.row_count());
}

} // namespace volap::execution

