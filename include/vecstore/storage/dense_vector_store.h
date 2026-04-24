#pragma once

#include <cstddef>
#include <string>

namespace vecstore::storage {

class DenseVectorStore
{
public:
    DenseVectorStore();
    ~DenseVectorStore();

    DenseVectorStore(const DenseVectorStore &) = delete;
    DenseVectorStore &operator=(const DenseVectorStore &) = delete;

    DenseVectorStore(DenseVectorStore &&other) noexcept;
    DenseVectorStore &operator=(DenseVectorStore &&other) noexcept;

    static void write_file(
        const std::string &path,
        const float *base,
        std::size_t count,
        std::size_t dim,
        bool normalized
    );

    void open_readonly(const std::string &path);
    void close() noexcept;

    bool is_open() const noexcept;

    const float *data() const noexcept;
    std::size_t count() const noexcept;
    std::size_t dim() const noexcept;
    bool normalized() const noexcept;

private:
    int fd_;
    void *mapping_;
    std::size_t mapping_size_;

    const float *data_;
    std::size_t count_;
    std::size_t dim_;
    bool normalized_;
};

} // namespace vecstore::storage
