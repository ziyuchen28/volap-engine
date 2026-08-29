
#include "test_util.h"

#include "volap/core/data_chunk.h"
#include "volap/execution/project.h"

namespace 
{

using namespace volap::execution;

DataChunk make_projection_input()
{
    DataChunk input;

    input.add_column(
        make_flat_vector<std::int64_t>({
            101,
            102,
            103
        })
    );

    input.add_column(
        make_flat_vector<double>({
            10.0,
            25.0,
            40.0
        })
    );

    input.add_column(
        make_flat_vector<double>({
            2.0,
            1.0,
            3.0
        })
    );

    return input;
}

void test_project_column_reference()
{
    DataChunk input = make_projection_input();
    DataChunk output;

    Project project({
        Projection::column(2),
        Projection::column(0)
    });

    project.execute(input, output);

    validate(output.column_count() == 2);
    validate(output.row_count() == 3);

    const double *f64_col = FlatVector::get_data<double>(output.column(0));
    const std::int64_t *i64_col = FlatVector::get_data<std::int64_t>(output.column(1));

    validate(f64_col[0] == 2.0, "f64 col row 0");
    validate(f64_col[2] == 3.0, "f64 col row 2");

    validate(i64_col[0] == 101, "i64 col row 0");
    validate(i64_col[2] == 103, "i64 col row 2");

    succeeded(__func__);
}

void test_project_f64_multiply()
{
    DataChunk input = make_projection_input();
    DataChunk output;

    Project project({
        Projection::multiply(1, 2)
    });

    project.execute(input, output);

    validate(output.column_count() == 1, "multiply column count");
    validate(output.row_count() == 3, "multiply row count");

    const double *amount =
        FlatVector::get_data<double>(output.column(0));

    validate(amount[0] == 20.0, "f64 row 0");
    validate(amount[1] == 25.0, "f64 row 1");
    validate(amount[2] == 120.0, "f64 row 2");

    succeeded(__func__);
}

void test_project_f32_multiply()
{
    DataChunk input;

    input.add_column(
        make_flat_vector<float>({
            1.5f,
            2.0f,
            4.0f
        })
    );

    input.add_column(
        make_flat_vector<float>({
            2.0f,
            3.0f,
            0.5f
        })
    );

    DataChunk output;

    Project project({
        Projection::multiply(0, 1)
    });

    project.execute(input, output);

    const float *result =
        FlatVector::get_data<float>(output.column(0));

    validate(result[0] == 3.0f, "f32 result row 0");
    validate(result[1] == 6.0f, "f32 result row 1");
    validate(result[2] == 2.0f, "f32 result row 2");

    succeeded(__func__);
}

void test_project_mixed_projections()
{
    DataChunk input = make_projection_input();
    DataChunk output;

    Project project({
        Projection::column(0),
        Projection::multiply(1, 2)
    });

    project.execute(input, output);

    const std::int64_t *ids = FlatVector::get_data<std::int64_t>(output.column(0));
    const double *multiplied_values = FlatVector::get_data<double>(output.column(1));

    validate(ids[0] == 101, "id row 0");
    validate(ids[2] == 103, "id row 2");

    validate(multiplied_values[0] == 20.0, "multipled values row 0");
    validate(multiplied_values[2] == 120.0, "multipled values row 2");

    succeeded(__func__);
}

} // anonynmous namespace


int main()
{
    test_project_column_reference();
}
