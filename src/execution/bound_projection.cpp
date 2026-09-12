#include "volap/execution/bound_projection.h"

#include <stdexcept>
#include <vector>

namespace volap::execution 
{

namespace 
{

using volap::core::DataSchema;
using volap::core::DataType;

BoundOperand bind_column(std::size_t column_index,
                         DataType execution_type,
                         const DataSchema &schema)
{
    const DataType source_type = schema.column_type(column_index);

    BoundCastType cast = BoundCastType::None;

    if (source_type != execution_type) 
    {
        if (source_type == DataType::Int64 && execution_type == DataType::Float32) {
            cast = BoundCastType::Int64ToFloat32;
        } else if (source_type == DataType::Int64 && execution_type == DataType::Float64) {
            cast = BoundCastType::Int64ToFloat64;
        } else if (source_type == DataType::Float32 && execution_type == DataType::Float64) {
            cast = BoundCastType::Float32ToFloat64;
        } else {
            throw std::invalid_argument(
                "Unsupported projection type casting");
        }
    }

    return BoundOperand 
    {
        column_index,
        source_type,
        execution_type,
        cast
    };
}

BoundProjection bind_column_ref(const Projection &expression,
                                const DataSchema &schema)
{
    const std::size_t column_index = expression.left_column_index();
    const DataType type = schema.column_type(column_index);

    return BoundProjection {
        BoundProjectionType::ColumnRef,
        type,
        BoundOperand {
            column_index,
            type,
            type,
            BoundCastType::None
        },
        {}
    };
}

DataType resolve_multiply_type(DataType left, DataType right)
{
    if (left == DataType::Float64 || right == DataType::Float64) {
        return DataType::Float64;
    }

    if (left == DataType::Float32 || right == DataType::Float32) {
        return DataType::Float32;
    }

    throw std::invalid_argument(
        "Unsupported multiply operand types");
}

// Resolve exectuion type based on types from left and right sides
BoundProjection bind_multiply(const Projection &expression, 
                              const DataSchema &schema)
{
    const std::size_t left_index = expression.left_column_index();
    const std::size_t right_index = expression.right_column_index();

    const DataType left_type = schema.column_type(left_index);
    const DataType right_type = schema.column_type(right_index);

    const DataType execution_type = resolve_multiply_type(left_type, right_type);

    BoundProjectionType bound_type;

    switch (execution_type) {
        case DataType::Float32:
            bound_type = BoundProjectionType::MultiplyFloat32;
            break;

        case DataType::Float64:
            bound_type = BoundProjectionType::MultiplyFloat64;
            break;

        default:
            throw std::logic_error(
                "Invalid multiply execution type");
    }

    return BoundProjection {
        bound_type,
        execution_type,
        bind_column(left_index, execution_type, schema),
        bind_column(right_index, execution_type, schema)
    };
}

} // namespace

std::vector<BoundProjection> bind_projections(const std::vector<Projection> &projections,
                                              const DataSchema &schema)
{
    std::vector<BoundProjection> bound;
    bound.reserve(projections.size());

    for (const Projection &projection : projections) {
        switch (projection.type()) {
            case ProjectionType::ColumnRef:
                bound.push_back(bind_column_ref(projection, schema));
                break;

            case ProjectionType::Multiply:
                bound.push_back(bind_multiply(projection, schema));
                break;
        }
    }

    return bound;
}

} // namespace volap::execution

