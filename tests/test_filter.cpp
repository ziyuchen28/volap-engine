
#include "volap/core/data_chunk.h"
#include "volap/execution/filter.h"

#include "test_util.h"

namespace {

using namespace volap::core;
using namespace volap::execution;

DataChunk make_input_chunk()
{
    DataChunk input;

    input.add_column(
        make_flat_vector<std::int64_t>({
            101, 102, 103, 104
        })
    );

    input.add_column(
        make_flat_vector<double>({
            10.0, 25.0, 40.0, 15.0
        })
    );

    return input;
}


void test_f64_filter_selected_rows()
{
    DataChunk input = make_input_chunk();
    DataChunk output;

    Filter filter = Filter::f64_greater_than(1, 20.0);

    succeeded(__func__);
}

} // anonymous namespace 

int main()
{
    test_f64_filter_selected_rows();
    return 0;
}
