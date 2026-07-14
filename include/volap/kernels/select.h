
#pragma once

#include "volap/core/column_view.h"
#include "volap/core/selection_vector.h"

namespace volap::kernels {

using namespace volap::core; 

// Greater than threshold
void select_i64_gt(const ColumnView &column, std::int64_t threshold, SelectionVector &out);
void select_f64_gt(const ColumnView &column, double threshold, SelectionVector &out);
void select_f32_gt(const ColumnView &column, float threshold, SelectionVector &out);


} // namespace volap::kernels
