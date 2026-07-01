
#include <volap/kernels/sum_product.h>

#include <stdexcept>
#include <random>
#include <chrono>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <cstdlib>

using namespace volap::kernels;

namespace {

std::string get_arg(int argc, char **argv, const std::string &key, const std::string &def)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }
    return def;
}

std::size_t parse_size_arg(int argc, char **argv, const std::string &key, const std::string &def)
{
    const std::string value = get_arg(argc, argv, key, def);
    try {
        return static_cast<std::size_t>(std::stoull(value));
    } catch (const std::exception&) {
        throw std::runtime_error("invalid numeric argument for " + key + ": " + value);
    }
}

KernelImpl parse_impl(const std::string &s)
{
    if (s == "scalar") {
        return KernelImpl::Scalar;
    }

    if (s == "avx2" || s == "avx2_fma") {
        return KernelImpl::Avx2Fma;
    }

    if (s == "auto") {
        return KernelImpl::Auto;
    }

    throw std::runtime_error("unknown --impl value: " + s);
}

template <typename T>
#if defined(__GNUC__) || defined(__clang__)
inline void do_not_optimize(const T &value)
{
    // Prevent dead code elim on value while not adding extra memory writes
    // compared to volatile T sink = value 
    // "memory" to prevent re-ordering code happens before or after
    __asm__ __volatile__("" : : "g"(value) : "memory");
}
#else
inline void do_not_optimize(const T &value)
{
    static volatile T sink;
    sink = value;
}
#endif


void bench_sum_product_f32(
    std::size_t rows,
    std::size_t iters,
    std::size_t warmup,
    const std::string &impl_name,
    const float *a,
    const float *b)
{
    const auto requested_impl = parse_impl(impl_name);
    for (std::size_t i = 0; i < warmup; ++i) {
        const float result = sum_product_f32(a, b, rows, requested_impl);
        do_not_optimize(result);
    }

    double checksum = 0.0;
    const auto t0 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iters; ++i) {
        const float result = sum_product_f32(a, b, rows, requested_impl);
        do_not_optimize(result);
        checksum += result;
    }
    const auto t1 = std::chrono::steady_clock::now();

    const double seconds = std::chrono::duration<double>(t1 - t0).count();
    const double ns_per_iter = seconds * 1e9 / static_cast<double>(iters);
    // Two input float columns are streamed. The output is one scalar aggregate.
    const double bytes_per_iter = static_cast<double>(2 * rows * sizeof(float));
    const double effective_gb_per_sec =
        (bytes_per_iter * static_cast<double>(iters)) / seconds / 1e9;

    std::cout << "benchmark=sum_product_f32\n";
    std::cout << "rows=" << rows << "\n";
    std::cout << "iters=" << iters << "\n";
    std::cout << "warmup=" << warmup << "\n";
    std::cout << "seconds=" << seconds << "\n";
    std::cout << "ns_per_iter=" << ns_per_iter << "\n";
    std::cout << "effective_gb_per_sec=" << effective_gb_per_sec << "\n";
    std::cout << "checksum=" << checksum << "\n";
}

}  //  namespace

int main(int argc, char **argv)
{
    const std::size_t rows = parse_size_arg(argc, argv, "--rows", "1048576");
    const std::size_t iters = parse_size_arg(argc, argv, "--iters", "1000");
    const std::size_t warmup = parse_size_arg(argc, argv, "--warmup", "20");


    if (rows == 0) {
        std::cerr << "--rows must be > 0\n";
        return 1;
    }

    if (iters == 0) {
        std::cerr << "--iters must be > 0\n";
        return 1;
    }

    std::mt19937 rng(42);
    // Scale to prevent floating point precision loss.
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<float> a(rows);
    std::vector<float> b(rows);

    for (std::size_t i = 0; i < rows; ++i) {
        a[i] = dist(rng);
        b[i] = dist(rng);
    }



    std::cout << "scalar\n";
    bench_sum_product_f32(rows, iters, warmup, "scalar", a.data(), b.data());

    std::cout << "avx2_fma\n";
    bench_sum_product_f32(rows, iters, warmup, "avx2_fma", a.data(), b.data());

    return 0;
}

