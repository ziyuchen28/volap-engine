

#include "test_util.h"

#include "volap/core/data_schema.h"
#include "volap/execution/bound_projection.h"
#include "volap/execution/project.h"

namespace 
{

using namespace volap::execution;
using namespace volap::core;


void test_bound_projection_with_cast()
{
    DataChunk input;
    // column 0: id
    input.add_column( make_flat_vector<std::int64_t>({ 101, 102, 103 }));
    // column 1: quantity
    input.add_column( make_flat_vector<std::int64_t>({ 1, 2, 3 }));
    // column 2: value
    input.add_column( make_flat_vector<double>({ 10.0, 20.0, 30.0 }));

    DataSchema schema = DataSchema::from_chunk(input);

    std::vector<Projection> projections {
        Projection::column(0),
        Projection::multiply(1, 2)
    };

    std::vector<BoundProjection> bound = bind_projections(projections, schema);

    validate(bound.size() == 2, "bound projection count");

    validate(
        bound[1].type == BoundProjectionType::MultiplyFloat64,
        "multiply bound to Float64");

    validate(
        bound[1].left.cast == BoundCastType::Int64ToFloat64,
        "left operand requires Int64 -> Float64");

    validate(
        bound[1].right.cast == BoundCastType::None,
        "right operand requires no cast");


    Project project(std::move(bound));

    DataChunk output;

    project.execute(input, output);


    const std::int64_t *ids = FlatVector::get_data<std::int64_t>(output.column(0));
    const double *total = FlatVector::get_data<double>(output.column(1));

    validate(ids[0] == 101, "id 0");
    validate(ids[1] == 102, "id 1");
    validate(ids[2] == 103, "id 2");

    validate(total[0] == 10.0, "total 0");
    validate(total[1] == 40.0, "total 1");
    validate(total[2] == 90.0, "total 2");

    succeeded(__func__);
}

} // anonynmous namespace


int main()
{
    test_bound_projection_with_cast();
}

