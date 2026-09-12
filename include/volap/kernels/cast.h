#pragma once

#include <cstddef>
#include <cstdint>

namespace volap::kernels {

void cast_i64_to_f32_scalar(const std::int64_t *input,
                            float *output,
                            std::size_t count) noexcept;

void cast_i64_to_f64_scalar(const std::int64_t *input,
                            double *output,
                            std::size_t count) noexcept;

void cast_f32_to_f64_scalar(const float *input,
                            double *output,
                            std::size_t count) noexcept;

} // namespace volap::kernels
