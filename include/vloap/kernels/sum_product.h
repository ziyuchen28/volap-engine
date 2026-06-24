


using SumProductFn = float (*)(const float*, const float*, std::size_t) noexcept

enum class KernelImpl 
{
    Auto,
    Scalar,
    Avx2Fma
};


float sum_product_f32(
    const float *a,
    const float *b,
    std::size_t n,
    KernelImpl impl = KernelImpl::Auto
) noexcept;
