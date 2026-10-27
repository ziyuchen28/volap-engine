

#include "test_util.h"

#include "volap/core/data_schema.h"
#include "volap/execution/bound_projection.h"
#include "volap/execution/project.h"

namespace 
{

using namespace volap::execution;
using namespace volap::core;


void test_project_with_cast()
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

void test_project_with_cast_schema_passed()
{
    DataChunk input;

    // id, quantity, value
    input.add_column(make_flat_vector<std::int64_t>({101, 102, 103}));
    input.add_column(make_flat_vector<std::int64_t>({2, 3, 4}));
    input.add_column(make_flat_vector<double>({10.0, 20.0, 30.0}));
    input.set_row_count(3);

    DataSchema schema = DataSchema::from_chunk(input);

    Project project({
        Projection::column(0),
        Projection::multiply(1, 2)
    }, schema);

    DataChunk output;
    project.execute(input, output);

    const std::int64_t *ids =
        FlatVector::get_data<std::int64_t>(output.column(0));

    const double *amounts =
        FlatVector::get_data<double>(output.column(1));

    validate(output.row_count() == 3, "first batch row count");
    validate(ids[0] == 101 && ids[1] == 102 && ids[2] == 103,
             "first batch ids");
    validate(amounts[0] == 20.0 &&
             amounts[1] == 60.0 &&
             amounts[2] == 120.0,
             "first batch multiplication");

    DataChunk next_input;

    next_input.add_column(make_flat_vector<std::int64_t>({201, 202}));
    next_input.add_column(make_flat_vector<std::int64_t>({5, 6}));
    next_input.add_column(make_flat_vector<double>({1.5, 2.5}));
    next_input.set_row_count(2);

    project.execute(next_input, output);

    ids = FlatVector::get_data<std::int64_t>(output.column(0));
    amounts = FlatVector::get_data<double>(output.column(1));

    validate(output.row_count() == 2, "second batch row count");
    validate(ids[0] == 201 && ids[1] == 202,
             "second batch ids");
    validate(amounts[0] == 7.5 && amounts[1] == 15.0,
             "second batch multiplication");

    succeeded(__func__);
}

} // anonynmous namespace


int main()
{
    test_project_with_cast();
    test_project_with_cast_schema_passed();
}

