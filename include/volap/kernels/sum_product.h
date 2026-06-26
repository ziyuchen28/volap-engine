
#pragma once
#include <cstddef>


namespace volap::kernels {

enum class KernelImpl 
{
    Auto,
    Scalar,
    Avx2Fma
};

using SumProductFn = float (*)(const float*, const float*, std::size_t) noexcept;

#if defined(VOLAP_BUILD_X86_AVX2_FMA)
float sum_product_f32_x86_avx2_fma(const float *a, const float *b, std::size_t n) noexcept;
#endif

float sum_product_f32_scalar(const float *a, const float *b, std::size_t n) noexcept;

const char *sum_product_f32_impl_name(KernelImpl impl) noexcept;

float sum_product_f32(
    const float *a,
    const float *b,
    std::size_t n,
    KernelImpl impl = KernelImpl::Auto
) noexcept;

} // namespace volap::kernels
