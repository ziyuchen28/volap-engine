
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

} // anonynmous namespace


int main()
{
    test_project_column_reference();
}
