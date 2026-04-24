#include "vecstore/storage/dense_vector_store.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>

namespace vecstore::storage {

namespace {

constexpr std::uint32_t k_dense_vector_store_version = 1;

struct DenseVectorHeader
{
    char magic[8];
    std::uint32_t version;
    std::uint32_t dim;
    std::uint64_t count;
    std::uint8_t normalized;
    std::uint8_t reserved[39];
};

static_assert(sizeof(DenseVectorHeader) == 64, "DenseVectorHeader must be 64 bytes");

constexpr char k_magic[8] = {'V', 'S', 'B', 'A', 'S', 'E', '0', '1'};


[[noreturn]] void throw_sys(const char *what)
{
    throw std::runtime_error(std::string(what) + ": " + std::strerror(errno));
}


void write_all_or_throw(int fd, const void *buf, std::size_t len)
{
    const char *p = static_cast<const char *>(buf);

    while (len > 0) {
        const ssize_t n = ::write(fd, p, len);
        if (n > 0) {
            p += n;
            len -= static_cast<std::size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        throw_sys("write");
    }
}


std::size_t checked_matrix_bytes(std::size_t count, std::size_t dim)
{
    if (count == 0 || dim == 0) {
        return 0;
    }

    // count * dim could wrap around, rearrange the check
    if (count > (static_cast<std::size_t>(-1) / dim)) {
        throw std::runtime_error("matrix size overflow");
    }

    const std::size_t elems = count * dim;

    if (elems > (static_cast<std::size_t>(-1) / sizeof(float))) {
        throw std::runtime_error("matrix byte size overflow");
    }

    return elems * sizeof(float);
}

} // namespace



DenseVectorStore::DenseVectorStore()
    : fd_(-1),
      mapping_(nullptr),
      mapping_size_(0),
      data_(nullptr),
      count_(0),
      dim_(0),
      normalized_(false) 
{}


DenseVectorStore::~DenseVectorStore()
{
    close();
}


DenseVectorStore::DenseVectorStore(DenseVectorStore &&other) noexcept
    : fd_(other.fd_),
      mapping_(other.mapping_),
      mapping_size_(other.mapping_size_),
      data_(other.data_),
      count_(other.count_),
      dim_(other.dim_),
      normalized_(other.normalized_)
{
    other.fd_ = -1;
    other.mapping_ = nullptr;
    other.mapping_size_ = 0;
    other.data_ = nullptr;
    other.count_ = 0;
    other.dim_ = 0;
    other.normalized_ = false;
}


DenseVectorStore &DenseVectorStore::operator=(DenseVectorStore &&other) noexcept
{
    if (this != &other) {
        close();

        fd_ = other.fd_;
        mapping_ = other.mapping_;
        mapping_size_ = other.mapping_size_;
        data_ = other.data_;
        count_ = other.count_;
        dim_ = other.dim_;
        normalized_ = other.normalized_;

        other.fd_ = -1;
        other.mapping_ = nullptr;
        other.mapping_size_ = 0;
        other.data_ = nullptr;
        other.count_ = 0;
        other.dim_ = 0;
        other.normalized_ = false;
    }

    return *this;
}


// count - number of vectors (db row counts)
void DenseVectorStore::write_file(const std::string &path,
                                 const float *base,
                                 std::size_t count,
                                 std::size_t dim,
                                 bool normalized)
{
    if (base == nullptr && count != 0 && dim != 0) {
        throw std::runtime_error("write_file: base is null");
    }

    const int fd = ::open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0) {
        throw_sys("open");
    }

    try {
        DenseVectorHeader hdr {};
        std::memcpy(hdr.magic, k_magic, sizeof(k_magic));
        hdr.version = k_dense_vector_store_version;
        hdr.dim = static_cast<std::uint32_t>(dim);
        hdr.count = static_cast<std::uint64_t>(count);
        hdr.normalized = normalized ? 1 : 0;

        write_all_or_throw(fd, &hdr, sizeof(hdr));

        const std::size_t matrix_bytes = checked_matrix_bytes(count, dim);
        if (matrix_bytes > 0) {
            write_all_or_throw(fd, base, matrix_bytes);
        }

        if (::fsync(fd) != 0) {
            throw_sys("fsync");
        }

        ::close(fd);
    } catch (...) {
        ::close(fd);
        throw;
    }
}


void DenseVectorStore::open_readonly(const std::string &path)
{
    close();

    fd_ = ::open(path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        throw_sys("open");
    }

    struct stat st {};
    if (::fstat(fd_, &st) != 0) {
        close();
        throw_sys("fstat");
    }

    /* fast initial best-effort check */
    if (st.st_size < static_cast<off_t>(sizeof(DenseVectorHeader))) {
        close();
        throw std::runtime_error("base vectors file invalid");
    }

    mapping_size_ = static_cast<std::size_t>(st.st_size);

    mapping_ = ::mmap(nullptr, mapping_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mapping_ == MAP_FAILED) {
        mapping_ = nullptr;
        close();
        throw_sys("mmap");
    }

    const auto *hdr = static_cast<const DenseVectorHeader *>(mapping_);

    if (std::memcmp(hdr->magic, k_magic, sizeof(k_magic)) != 0) {
        close();
        throw std::runtime_error("bad base vectors magic");
    }

    if (hdr->version != k_dense_vector_store_version) {
        close();
        throw std::runtime_error("unsupported base vectors version");
    }

    dim_ = static_cast<std::size_t>(hdr->dim);
    count_ = static_cast<std::size_t>(hdr->count);
    normalized_ = (hdr->normalized != 0);

    const std::size_t matrix_bytes = checked_matrix_bytes(count_, dim_);
    const std::size_t expected_size = sizeof(DenseVectorHeader) + matrix_bytes;

    if (expected_size != mapping_size_) {
        close();
        throw std::runtime_error("base vectors size/header mismatch");
    }

    data_ = reinterpret_cast<const float *>(
        static_cast<const char *>(mapping_) + sizeof(DenseVectorHeader)
    );
}


void DenseVectorStore::close() noexcept
{
    if (mapping_ != nullptr) {
        ::munmap(mapping_, mapping_size_);
    }

    if (fd_ >= 0) {
        ::close(fd_);
    }

    fd_ = -1;
    mapping_ = nullptr;
    mapping_size_ = 0;
    data_ = nullptr;
    count_ = 0;
    dim_ = 0;
    normalized_ = false;
}


bool DenseVectorStore::is_open() const noexcept
{
    return data_ != nullptr;
}


const float *DenseVectorStore::data() const noexcept
{
    return data_;
}


std::size_t DenseVectorStore::count() const noexcept
{
    return count_;
}


std::size_t DenseVectorStore::dim() const noexcept
{
    return dim_;
}

} // namespace vecstore::storage


