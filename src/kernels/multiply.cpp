#include "volap/kernels/multiply.h"

namespace volap::kernels {

void multiply_f32_scalar(const float *left,
                         const float *right,
                         float *output,
                         std::size_t count) noexcept
{
    for (std::size_t i = 0; i < count; ++i) {
        output[i] = left[i] * right[i];
    }
}

void multiply_f64_scalar(const double *left,
                         const double *right,
                         double *output,
                         std::size_t count) noexcept
{
    for (std::size_t i = 0; i < count; ++i) {
        output[i] = left[i] * right[i];
    }
}

} // namespace volap::kernels
