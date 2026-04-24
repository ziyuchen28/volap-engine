#include "vecstore/flat/search.h"
#include "vecstore/vecmath/normalize.h"
#include "vecstore/storage/dense_vector_store.h"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

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

enum class SearchStorage
{
    Ram,
    MMap
};

enum class SearchMetric
{
    Dot,
    Cosine
};

SearchStorage parse_storage(const std::string &s)
{
    if (s == "mmap") {
        return SearchStorage::MMap;
    }
    return SearchStorage::Ram;
}

const char *storage_name(SearchStorage s)
{
    switch (s) {
        case SearchStorage::MMap:
            return "mmap";
        case SearchStorage::Ram:
        default:
            return "ram";
    }
}

SearchMetric parse_metric(const std::string &s)
{
    if (s == "cosine") {
        return SearchMetric::Cosine;
    }
    return SearchMetric::Dot;
}

const char *metric_name(SearchMetric m)
{
    switch (m) {
        case SearchMetric::Cosine:
            return "cosine";
        case SearchMetric::Dot:
        default:
            return "dot";
    }
}

vecstore::vecmath::DotImpl parse_kernel_impl(const std::string &s)
{
    using vecstore::vecmath::DotImpl;
    if (s == "scalar") {
        return DotImpl::Scalar;
    }
    if (s == "avx2") {
        return DotImpl::Avx2;
    }
    return DotImpl::Auto;
}


} // namespace


int main(int argc, char **argv)
{
    // db row counts
    const std::size_t count =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--count", "20000")));
    const std::size_t dim =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--dim", "1536")));
    const std::size_t k =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--k", "10")));
    const std::size_t query_count =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--query_count", "32")));
    const std::size_t iters =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--iters", "20")));
    const std::size_t warmup =
        static_cast<std::size_t>(std::stoull(get_arg(argc, argv, "--warmup", "2")));
    const SearchStorage storage =
        parse_storage(get_arg(argc, argv, "--storage", "ram"));
    const SearchMetric search_metric =
        parse_metric(get_arg(argc, argv, "--search-metric", "dot"));
    const std::string path =
        get_arg(argc, argv, "--path", "/tmp/vecstore_flat_search.bin");
    const auto requested_kernel_impl =
        parse_kernel_impl(get_arg(argc, argv, "--kernel-impl", "auto"));
    const auto resolved_kernel_impl =
        vecstore::vecmath::resolve_dot_impl(requested_kernel_impl);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<float> base(count * dim);
    std::vector<float> query_bank(query_count * dim);

    for (float &x : base) {
        x = dist(rng);
    }

    for (float &x : query_bank) {
        x = dist(rng);
    }

    const bool normalize = (search_metric == SearchMetric::Cosine);
    if (normalize) {
        vecstore::vecmath::normalize_rows_l2_inplace(base.data(), count, dim);
        vecstore::vecmath::normalize_rows_l2_inplace(query_bank.data(), query_count, dim);
    }

    // base vectors
    const float *search_base = nullptr;
    vecstore::storage::DenseVectorStore mapped;
    if (storage == SearchStorage::MMap) {
        vecstore::storage::DenseVectorStore::write_file(path, base.data(), count, dim, normalize);
        mapped.open_readonly(path);
        search_base = mapped.data();
    } else {
        search_base = base.data();
    }

    // search
    std::vector<vecstore::flat::SearchResult> out;
    // sink: make sure search_topk actually run and not removed from compiler dead code elim
    // volaile: makle sure the loop runs iters time to avoid loop hoisting
    volatile float sink = 0.0f;
    for (std::size_t iter = 0; iter < warmup; ++iter) {
        for (std::size_t q = 0; q < query_count; ++q) {
            const float *query = query_bank.data() + q * dim;

            vecstore::flat::search_topk(
                search_base,
                count,
                dim,
                query,
                k,
                out,
                requested_kernel_impl
            );

            if (!out.empty()) {
                sink += out[0].score;
            }
        }
    }
    const auto t0 = std::chrono::steady_clock::now();

    for (std::size_t iter = 0; iter < iters; ++iter) {
        for (std::size_t q = 0; q < query_count; ++q) {
            const float *query = query_bank.data() + q * dim;

            vecstore::flat::search_topk(
                search_base,
                count,
                dim,
                query,
                k,
                out,
                requested_kernel_impl
            );

            if (!out.empty()) {
                sink += out[0].score;
            }
        }
    }

    const auto t1 = std::chrono::steady_clock::now();

    const std::size_t total_queries = iters * query_count;
    const double sec = std::chrono::duration<double>(t1 - t0).count();
    const double ns_per_query = sec * 1e9 / static_cast<double>(total_queries);
    const double qps = static_cast<double>(total_queries) / sec;
    // how many database vectors we compare against query_count per second;
    const double vectors_per_sec = static_cast<double>(count) * qps;

    // ignoring the query vector   
    const double bytes_per_query = static_cast<double>(count * dim * sizeof(float));
    const double effective_gb_per_sec = (bytes_per_query * qps) / 1e9;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "benchmark=flat_search\n";
    std::cout << "storage=" << storage_name(storage) << "\n";
    std::cout << "metric=" << metric_name(search_metric) << "\n";
    std::cout << "requested_kernel_impl=" << vecstore::vecmath::dot_impl_name(requested_kernel_impl) << "\n";
    std::cout << "resolved_kernel_impl=" << vecstore::vecmath::dot_impl_name(resolved_kernel_impl) << "\n";
    std::cout << "count=" << count << "\n";
    std::cout << "dim=" << dim << "\n";
    std::cout << "k=" << k << "\n";
    std::cout << "query_count=" << query_count << "\n";
    std::cout << "iters=" << iters << "\n";
    std::cout << "seconds=" << sec << "\n";
    std::cout << "ns_per_query=" << ns_per_query << "\n";
    std::cout << "queries_per_sec=" << qps << "\n";
    std::cout << "vectors_scanned_per_sec=" << vectors_per_sec << "\n";
    std::cout << "effective_gb_per_sec=" << effective_gb_per_sec << "\n";
    std::cout << "top1_index=" << (out.empty() ? 0 : out[0].index) << "\n";
    std::cout << "top1_score=" << (out.empty() ? 0.0f : out[0].score) << "\n";
    std::cout << "sink=" << sink << "\n";

    return 0;
}


