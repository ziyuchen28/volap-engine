
#include <immintrin.h>
#include <cstddef>

namespace volap::kernels {


// benchmark result shows regressions using compared to acc4 only version
// likely reason: hadd is slow
// it's converted into multiple cpu instructions and blocks vector alus from other cycles
// static inline float hsum256_ps_hadd(__m256 v) noexcept
// {
//     const __m128 low = _mm256_castps256_ps128(v);
//     const __m128 high = _mm256_extractf128_ps(v, 1);
//     __m128 sum = _mm_add_ps(low, high);
//     sum = _mm_hadd_ps(sum, sum);
//     sum = _mm_hadd_ps(sum, sum);
//     return _mm_cvtss_f32(sum);
//
// }

static inline float hsum256_ps_shuffle(__m256 v) noexcept
{
    // fold 256 -> 128 
    const __m128 vlow = _mm256_castps256_ps128(v);
    const __m128 vhigh = _mm256_extractf128_ps(v, 1);
    __m128 sum = _mm_add_ps(vlow, vhigh);

    // fold 128 -> 64
    __m128 high64 = _mm_movehl_ps(sum, sum);
    sum = _mm_add_ps(sum, high64);

    // fold 64 -> 32
    // 0x55 (01010101) grabs the 2nd float into every slot
    __m128 high32 = _mm_shuffle_ps(sum, sum, 0x55);
    sum = _mm_add_ps(sum, high32);

    return _mm_cvtss_f32(sum);
}


// optimization 1: four acc loop unrolling
//   make use of most ymm regisers (but below 16 to avoid ymm spilling) 
// optimization 2: hadd
//   in-register hotizontal sum, strictly kept in register, avoid cpu cache store load cycles
// optimization 3: fma 
//   fma instead of separate mul and add
float sum_product_f32_x86_avx2_fma(
    const float *a, 
    const float *b, 
    std::size_t n
) noexcept
{
    __m256 acc0 = _mm256_setzero_ps(); 
    __m256 acc1 = _mm256_setzero_ps(); 
    __m256 acc2 = _mm256_setzero_ps(); 
    __m256 acc3 = _mm256_setzero_ps(); 

    std::size_t i = 0;

    for (; i + 32 <= n; i += 32) {
        const __m256 va0 = _mm256_loadu_ps(a + i);
        const __m256 va1 = _mm256_loadu_ps(a + i + 8);
        const __m256 va2 = _mm256_loadu_ps(a + i + 16);
        const __m256 va3 = _mm256_loadu_ps(a + i + 24);

        const __m256 vb0 = _mm256_loadu_ps(b + i);
        const __m256 vb1 = _mm256_loadu_ps(b + i + 8);
        const __m256 vb2 = _mm256_loadu_ps(b + i + 16);
        const __m256 vb3 = _mm256_loadu_ps(b + i + 24);

        // vfmadd231ps ymm0, ymm4, ymm8
        acc0 = _mm256_fmadd_ps(va0, vb0, acc0);
        // vfmadd231ps ymm1, ymm5, ymm9
        acc1 = _mm256_fmadd_ps(va1, vb1, acc1);
        // vfmadd231ps ymm2, ymm6, ymm10
        acc2 = _mm256_fmadd_ps(va2, vb2, acc2);
        // vfmadd231ps ymm3, ymm7, ymm11
        acc3 = _mm256_fmadd_ps(va3, vb3, acc3);

    }

    // vaddps ymm0, ymm0, ymm1
    // vaddps ymm2, ymm2, ymm3
    // vaddps ymm0, ymm0, ymm2
    __m256 acc = _mm256_add_ps(_mm256_add_ps(acc0, acc1), _mm256_add_ps(acc2, acc3));


    for (; i + 8 <= n; i += 8) {
        const __m256 va = _mm256_loadu_ps(a + i);
        const __m256 vb = _mm256_loadu_ps(b + i);
        acc = _mm256_add_ps(acc, _mm256_mul_ps(va, vb));
    }

    float sum = hsum256_ps_shuffle(acc);

    for (; i < n; ++i) {
        sum += a[i] * b[i];
    }

    return sum;
}



} // namespace volap::kernels
