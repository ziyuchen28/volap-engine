

#include "test_util.h"

#include "volap/core/data_schema.h"
#include "volap/execution/bound_projection.h"

namespace 
{

using namespace volap::execution;
using namespace volap::core;

void test_bound_projection()
{
    DataSchema schema({
        Type::Int64,
        Type::Float64,
        Type::Float32
    });

    std::vector<Projection> projections {
        Projection::column(0),
        Projection::multiply(1, 2)
    };

    const std::vector<BoundProjection> bound =
        bind_projections(projections, schema);

    validate(bound.size() == 2, "bound projection count");

    // column(0)
    validate(bound[0].type == BoundProjectionType::ColumnRef,
             "column ref type");

    validate(bound[0].result_type == Type::Int64,
             "column ref result type");

    validate(bound[0].left.column_index == 0,
             "column ref index");

    validate(bound[0].left.source_type == Type::Int64,
             "column ref source type");

    validate(bound[0].left.execution_type == Type::Int64,
             "column ref execution type");

    validate(bound[0].left.cast == BoundCastType::None,
             "column ref cast");

    // multiply(1, 2)
    validate(bound[1].type == BoundProjectionType::MultiplyFloat64,
             "multiply type");

    validate(bound[1].result_type == Type::Float64,
             "multiply result type");

    validate(bound[1].left.column_index == 1,
             "multiply left index");

    validate(bound[1].left.source_type == Type::Float64,
             "multiply left source type");

    validate(bound[1].left.execution_type == Type::Float64,
             "multiply left execution type");

    validate(bound[1].left.cast == BoundCastType::None,
             "multiply left cast");

    validate(bound[1].right.column_index == 2,
             "multiply right index");

    validate(bound[1].right.source_type == Type::Float32,
             "multiply right source type");

    validate(bound[1].right.execution_type == Type::Float64,
             "multiply right execution type");

    validate(bound[1].right.cast == BoundCastType::Float32ToFloat64,
             "multiply right cast");

    succeeded(__func__);
}

} // anonynmous namespace


int main()
{
    test_bound_projection();
}

