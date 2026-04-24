#include "vecstore/flat/search.h"
#include "vecstore/storage/dense_vector_store.h"
#include "vecstore/vecmath/normalize.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {


void fail(const char *msg)
{
    std::cerr << msg << "\n";
    std::exit(1);
}


void expect_close(float a, float b, float tol, const char *msg)
{
    if (std::fabs(a - b) > tol) {
        std::cerr
            << msg
            << ": got=" << a
            << " expected=" << b
            << " tol=" << tol
            << "\n";
        std::exit(1);
    }
}


void test_roundtrip_and_search()
{
    const std::string path = "/tmp/vecstore_test.bin";

    constexpr std::size_t count = 4;
    constexpr std::size_t dim = 4;

    std::vector<float> base {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f
    };

    vecstore::vecmath::normalize_rows_l2_inplace(base.data(), count, dim);
    vecstore::storage::DenseVectorStore::write_file(path, base.data(), count, dim, true);

    vecstore::storage::DenseVectorStore db;
    db.open_readonly(path);

    if (!db.is_open()) {
        fail("db should be open");
    }

    if (db.count() != count) {
        fail("count mismatch");
    }

    if (db.dim() != dim) {
        fail("dim mismatch");
    }

    static constexpr float query[4] {1.0f, 0.0f, 0.0f, 0.0f};
    std::vector<vecstore::flat::SearchResult> out;

    vecstore::flat::search_topk(
        db.data(),
        db.count(),
        db.dim(),
        query,
        2,
        out,
        vecstore::vecmath::DotImpl::Scalar
    );

    if (out.size() != 2) {
        fail("expected 2 results");
    }

    if (out[0].index != 0) {
        fail("expected top1 index == 0");
    }

    if (out[1].index != 3) {
        fail("expected top2 index == 3");
    }

    expect_close(out[0].score, 1.0f, 1e-6f, "top1 score mismatch");
    expect_close(out[1].score, 0.707107f, 1e-6f, "top2 score mismatch"); // 0.5f, 0.5f, 0.0f, 0.0f 
                                                                        
    std::filesystem::remove(path);
}

} // namespace


int main()
{
    test_roundtrip_and_search();

    std::cout << "OK: test_corpus_mmap\n";
    return 0;
}


