
#pragma once

#include "volap/core/selection_vector.h"
#include "volap/core/vector.h"

namespace volap::kernels {

using namespace volap::core; 

// Greater than threshold
void select_i64_gt(const Vector &input, std::int64_t threshold, SelectionVector &out);
void select_f64_gt(const Vector &input, double threshold, SelectionVector &out);
void select_f32_gt(const Vector &input, float threshold, SelectionVector &out);


} // namespace volap::kernels
