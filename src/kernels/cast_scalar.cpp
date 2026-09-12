#include "volap/kernels/cast.h"

namespace volap::kernels {

void cast_i64_to_f32_scalar(const std::int64_t *input,
                            float *output,
                            std::size_t count) noexcept
{
    for (std::size_t i = 0; i < count; ++i) {
        output[i] = static_cast<float>(input[i]);
    }
}

void cast_i64_to_f64_scalar(const std::int64_t *input,
                            double *output,
                            std::size_t count) noexcept
{
    for (std::size_t i = 0; i < count; ++i) {
        output[i] = static_cast<double>(input[i]);
    }
}

void cast_f32_to_f64_scalar(const float *input,
                            double *output,
                            std::size_t count) noexcept
{
    for (std::size_t i = 0; i < count; ++i) {
        output[i] = static_cast<double>(input[i]);
    }
}

} // namespace volap::kernels
