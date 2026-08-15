

#include "volap/kernels/select.h"
#include "volap/core/flat_vector.h"


namespace volap::kernels 

{

namespace 
{


template <typename T>
void select_gt_scalar(
    const T *values,
    std::size_t count,
    T threshold,
    SelectionVector &output)
{
    output.clear();
    output.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        if (values[i] > threshold) {
            output.push_back(i);
        }
    }
}

} // anonymous namespace

void select_i64_gt(const Vector &input, std::int64_t threshold, SelectionVector &out)
{
    // hard code to flat vector for now
    // we should gneralize this into uniformed format once other encodings are suported 
    const auto *values = FlatVector::get_data<std::int64_t>(input);

    select_gt_scalar(values, input.size(), threshold, out);
}


void select_f64_gt(const Vector &input, double threshold, SelectionVector &out)
{
    // hard code to flat vector for now
    // we should gneralize this into uniformed format once other encodings are suported 
    const auto *values = FlatVector::get_data<double>(input);

    select_gt_scalar(values, input.size(), threshold, out);
}


void select_f32_gt(const Vector &input, float threshold, SelectionVector &out)
{
    // hard code to flat vector for now
    // we should gneralize this into uniformed format once other encodings are suported 
    const auto *values = FlatVector::get_data<float>(input);

    select_gt_scalar(values, input.size(), threshold, out);
}


} // namespace volap::kernels
