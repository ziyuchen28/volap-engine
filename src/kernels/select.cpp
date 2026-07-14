

#include "volap/kernels/select.h"


namespace volap::kernels {


void select_i64_gt(const ColumnView &column, std::int64_t threshold, SelectionVector &out)
{
    const auto values = column.as_span<std::int64_t>();

    out.clear();
    out.reserve(values.size());

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (values[i] > threshold) {
            out.push_back(i);
        }
    }
}


void select_f64_gt(const ColumnView &column, double threshold, SelectionVector &out)
{
    const auto values = column.as_span<double>();

    out.clear();
    out.reserve(values.size());

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (values[i] > threshold) {
            out.push_back(i);
        }
    }
}


void select_f32_gt(const ColumnView &column, float threshold, SelectionVector &out)
{
    const auto values = column.as_span<float>();

    out.clear();
    out.reserve(values.size());

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (values[i] > threshold) {
            out.push_back(i);
        }
    }
}


} // namespace volap::kernels
