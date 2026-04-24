#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace vecstore::storage {

class MetadataStore
{
public:
    MetadataStore();
    ~MetadataStore();

    MetadataStore(const MetadataStore &) = delete;
    MetadataStore &operator=(const MetadataStore &) = delete;

    MetadataStore(MetadataStore &&other) noexcept;
    MetadataStore &operator=(MetadataStore &&other) noexcept;

    static void write_file(
        const std::string &path,
        const std::vector<std::uint64_t> &ids,
        const std::vector<std::string> &payloads
    );

    void open_readonly(const std::string &path);
    void close() noexcept;

    bool is_open() const noexcept;
    std::size_t count() const noexcept;

    bool lookup(
        std::size_t row_index,
        std::uint64_t &id_out,
        std::string_view &payload_out
    ) const noexcept;

private:
    int fd_;
    void *mapping_;
    std::size_t mapping_size_;

    struct Entry;
    const Entry *entries_;
    const char *blob_;
    std::size_t blob_size_;
    std::size_t count_;
};

} // namespace vecstore::storage
