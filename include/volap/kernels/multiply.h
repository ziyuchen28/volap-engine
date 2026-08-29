#pragma once

#include <cstddef>

namespace volap::kernels {

void multiply_f32_scalar(const float *left,
                         const float *right,
                         float *output,
                         std::size_t count) noexcept;

void multiply_f64_scalar(const double *left,
                         const double *right,
                         double *output,
                         std::size_t count) noexcept;

} // namespace volap::kernels
