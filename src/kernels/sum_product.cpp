
#include <volap/kernels/sum_product.h>
#include <iostream>


namespace volap::kernels {

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
// Arch check already exists in cmake build, adding a runtime check to guard cross-compilation case 
#define VOLAP_X86_RUNTIME_DETECT 1
#else
#define VOLAP_X86_RUNTIME_DETECT 0
#endif

static KernelImpl detect_real_optimal_sum_product_f32_impl() noexcept
{
// guard with runtime check to ensure target machine is x86   
#if VOLAP_X86_RUNTIME_DETECT && defined(VOLAP_BUILD_X86_AVX2_FMA) && (defined(__GNUC__) || defined(__clang__))
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma")) {
        // std::cout << "av2 + fma" << std::endl;
        return KernelImpl::Avx2Fma;
    }
#endif
    return KernelImpl::Scalar;
}


static KernelImpl resolve_sum_product_f32_impl(KernelImpl impl) noexcept
{
    if (impl == KernelImpl::Auto) {
        return detect_real_optimal_sum_product_f32_impl();
    }
    if (impl == KernelImpl::Avx2Fma) {
#if defined(VOLAP_BUILD_X86_AVX2_FMA)
        if (detect_real_optimal_sum_product_f32_impl() == KernelImpl::Avx2Fma) {
            return KernelImpl::Avx2Fma;
        }
#endif
        return KernelImpl::Scalar;
    }
    return KernelImpl::Scalar;
}


static SumProductFn resolve_sum_product_f32_function(KernelImpl impl) noexcept
{
    const KernelImpl resolved = resolve_sum_product_f32_impl(impl);
    switch (resolved) {
        case KernelImpl::Avx2Fma:
#if defined(VOLAP_BUILD_X86_AVX2_FMA)
            return &sum_product_f32_x86_avx2_fma;
#else
            return &sum_product_f32_scalar;
#endif
        case KernelImpl::Scalar:
        case KernelImpl::Auto:
        default:
            return &sum_product_f32_scalar;
    }
}


float sum_product_f32(
    const float *a,
    const float *b,
    std::size_t n,
    KernelImpl impl
) noexcept
{
    // if (a.size() != b.size()) {
    //     throw std::invalid_argument("sum_product_f32: column size mismatch");
    // }
    const SumProductFn fn = resolve_sum_product_f32_function(impl);
    return fn(a, b, n);
}

} // namespace volap::kernels
