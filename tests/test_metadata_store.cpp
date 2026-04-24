#include "vecstore/flat/search.h"
#include "vecstore/storage/metadata_store.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
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


void test_meta_roundtrip()
{
    const std::string path = "/tmp/vecstore_test_meta.bin";

    const std::vector<std::uint64_t> ids {
        101,
        202,
        303
    };

    const std::vector<std::string> payloads {
        "alpha",
        "beta",
        "gamma"
    };

    vecstore::storage::MetadataStore::write_file(path, ids, payloads);
    
    vecstore::storage::MetadataStore meta;
    meta.open_readonly(path);

    if (!meta.is_open()) {
        fail("meta should be open");
    }

    if (meta.count() != ids.size()) {
        fail("meta count mismatch");
    }

    for (std::size_t i = 0; i < ids.size(); ++i) {
        std::uint64_t id = 0;
        std::string_view payload;

        if (!meta.lookup(i, id, payload)) {
            fail("lookup failed");
        }

        if (id != ids[i]) {
            fail("id mismatch");
        }

        if (payload != payloads[i]) {
            fail("payload mismatch");
        }
    }

    std::filesystem::remove(path);
}


void test_search_plus_meta()
{
    const std::string path = "/tmp/vecstore_test_meta_search.bin";

    const std::size_t count = 4;
    const std::size_t dim = 4;

    const std::vector<float> dense {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f
    };

    const std::vector<std::uint64_t> ids {
        1001,
        1002,
        1003,
        1004
    };

    const std::vector<std::string> payloads {
        "vec-a",
        "vec-b",
        "vec-c",
        "vec-d"
    };

    vecstore::storage::MetadataStore::write_file(path, ids, payloads);

    vecstore::storage::MetadataStore meta;
    meta.open_readonly(path);

    const float query[4] {1.0f, 0.0f, 0.0f, 0.0f};

    std::vector<vecstore::flat::SearchResult> out;

    vecstore::flat::search_topk(
        dense.data(),
        count,
        dim,
        query,
        2,
        out,
        vecstore::vecmath::DotImpl::Auto
    );

    if (out.size() != 2) {
        fail("expected 2 search results");
    }

    if (out[0].index != 0 || out[1].index != 3) {
        fail("unexpected search ordering");
    }

    expect_close(out[0].score, 1.0f, 1e-6f, "top1 score mismatch");
    expect_close(out[1].score, 0.5f, 1e-6f, "top2 score mismatch");

    {
        std::uint64_t id = 0;
        std::string_view payload;

        if (!meta.lookup(out[0].index, id, payload)) {
            fail("lookup top1 failed");
        }

        if (id != 1001 || payload != "vec-a") {
            fail("top1 meta mismatch");
        }
    }

    {
        std::uint64_t id = 0;
        std::string_view payload;

        if (!meta.lookup(out[1].index, id, payload)) {
            fail("lookup top2 failed");
        }

        if (id != 1004 || payload != "vec-d") {
            fail("top2 meta mismatch");
        }
    }

    std::filesystem::remove(path);
}

} // namespace

int main()
{
    test_meta_roundtrip();
    test_search_plus_meta();

    std::cout << "OK: test_meta\n";
    return 0;
}
