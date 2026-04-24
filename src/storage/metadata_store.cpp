#include "vecstore/storage/metadata_store.h"

#include <cerrno>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>

namespace vecstore::storage {

namespace {

constexpr std::uint32_t k_metadata_version = 1;

struct MetadataHeader
{
    char magic[8];
    std::uint32_t version;
    std::uint32_t reserved0;
    std::uint64_t count;
    std::uint64_t entries_offset;
    std::uint64_t blob_offset;
    // pad to 64 bytes
    // 1 to make sure data is read in next cache line in full
    // 2 room for adding more metadata later
    std::uint8_t reserved[24];
    // extend to 128 / 256 / larger? 
};

static_assert(sizeof(MetadataHeader) == 64, "MetadataHeader must be 64 bytes");

constexpr char k_magic[8] = {'V', 'S', 'M', 'E', 'T', 'A', '0', '1'};

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


std::size_t checked_add(std::size_t a, std::size_t b)
{
    if (a > SIZE_MAX - b) {
        throw std::runtime_error("size overflow");
    }

    return a + b;
}

} // namespace


struct MetadataStore::Entry
{
    std::uint64_t id;
    std::uint64_t payload_offset;
    std::uint64_t payload_size;
};


MetadataStore::MetadataStore()
    : fd_(-1),
      mapping_(nullptr),
      mapping_size_(0),
      entries_(nullptr),
      blob_(nullptr),
      blob_size_(0),
      count_(0)
{}


MetadataStore::~MetadataStore()
{
    close();
}

MetadataStore::MetadataStore(MetadataStore &&other) noexcept
    : fd_(other.fd_),
      mapping_(other.mapping_),
      mapping_size_(other.mapping_size_),
      entries_(other.entries_),
      blob_(other.blob_),
      blob_size_(other.blob_size_),
      count_(other.count_)
{
    other.fd_ = -1;
    other.mapping_ = nullptr;
    other.mapping_size_ = 0;
    other.entries_ = nullptr;
    other.blob_ = nullptr;
    other.blob_size_ = 0;
    other.count_ = 0;
}


MetadataStore &MetadataStore::operator=(MetadataStore &&other) noexcept
{
    if (this != &other) {
        close();

        fd_ = other.fd_;
        mapping_ = other.mapping_;
        mapping_size_ = other.mapping_size_;
        entries_ = other.entries_;
        blob_ = other.blob_;
        blob_size_ = other.blob_size_;
        count_ = other.count_;

        other.fd_ = -1;
        other.mapping_ = nullptr;
        other.mapping_size_ = 0;
        other.entries_ = nullptr;
        other.blob_ = nullptr;
        other.blob_size_ = 0;
        other.count_ = 0;
    }

    return *this;
}


void MetadataStore::write_file(
    const std::string &path,
    const std::vector<std::uint64_t> &ids,
    const std::vector<std::string> &payloads)
{
    if (ids.size() != payloads.size()) {
        throw std::runtime_error("ids.size() must equal payloads.size()");
    }

    const std::size_t count = ids.size();
    const std::size_t entries_bytes = count * sizeof(Entry);

    std::size_t blob_bytes = 0;
    for (const auto &payload : payloads) {
        blob_bytes = checked_add(blob_bytes, payload.size());
    }

    const std::size_t entries_offset = sizeof(MetadataHeader);
    const std::size_t blob_offset = checked_add(entries_offset, entries_bytes);

    int fd = ::open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0) {
        throw_sys("open");
    }

    try {
        MetadataHeader hdr {};
        std::memcpy(hdr.magic, k_magic, sizeof(k_magic));
        hdr.version = k_metadata_version;
        hdr.count = static_cast<std::uint64_t>(count);
        hdr.entries_offset = static_cast<std::uint64_t>(entries_offset);
        hdr.blob_offset = static_cast<std::uint64_t>(blob_offset);

        write_all_or_throw(fd, &hdr, sizeof(hdr));

        std::uint64_t running_blob_offset = 0;

        for (std::size_t i = 0; i < count; ++i) {
            Entry e {};
            e.id = ids[i];
            e.payload_offset = running_blob_offset;
            e.payload_size = static_cast<std::uint64_t>(payloads[i].size());

            write_all_or_throw(fd, &e, sizeof(e));

            running_blob_offset += e.payload_size;
        }

        for (const auto &payload : payloads) {
            if (!payload.empty()) {
                write_all_or_throw(fd, payload.data(), payload.size());
            }
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


void MetadataStore::open_readonly(const std::string &path)
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

    if (st.st_size < static_cast<off_t>(sizeof(MetadataHeader))) {
        close();
        throw std::runtime_error("meta file too small");
    }

    mapping_size_ = static_cast<std::size_t>(st.st_size);

    mapping_ = ::mmap(nullptr, mapping_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mapping_ == MAP_FAILED) {
        mapping_ = nullptr;
        close();
        throw_sys("mmap");
    }

    const auto *hdr = static_cast<const MetadataHeader*>(mapping_);

    if (std::memcmp(hdr->magic, k_magic, sizeof(k_magic)) != 0) {
        close();
        throw std::runtime_error("bad meta magic");
    }

    if (hdr->version != 1) {
        close();
        throw std::runtime_error("unsupported meta version");
    }

    count_ = static_cast<std::size_t>(hdr->count);
    const std::size_t entries_offset = static_cast<std::size_t>(hdr->entries_offset);
    const std::size_t blob_offset = static_cast<std::size_t>(hdr->blob_offset);
    const std::size_t entries_bytes = count_ * sizeof(Entry);

    // safety check for broken/crashed write edge case
    if (entries_offset != sizeof(MetadataHeader)) {
        close();
        throw std::runtime_error("unexpected entries offset");
    }
    if (blob_offset != checked_add(entries_offset, entries_bytes)) {
        close();
        throw std::runtime_error("meta blob offset mismatch");
    }
    if (blob_offset > mapping_size_) {
        close();
        throw std::runtime_error("meta blob offset out of range");
    }

    entries_ = reinterpret_cast<const Entry *>(
        static_cast<const char *>(mapping_) + entries_offset
    );

    blob_ = static_cast<const char *>(mapping_) + blob_offset;
    blob_size_ = mapping_size_ - blob_offset;

    for (std::size_t i = 0; i < count_; ++i) {
        const Entry &e = entries_[i];

        if (e.payload_offset > blob_size_) {
            close();
            throw std::runtime_error("meta payload offset out of range");
        }

        if (e.payload_size > blob_size_ - static_cast<std::size_t>(e.payload_offset)) {
            close();
            throw std::runtime_error("meta payload size out of range");
        }
    }
}

void MetadataStore::close() noexcept
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
    entries_ = nullptr;
    blob_ = nullptr;
    blob_size_ = 0;
    count_ = 0;
}

bool MetadataStore::is_open() const noexcept
{
    return entries_ != nullptr;
}

std::size_t MetadataStore::count() const noexcept
{
    return count_;
}


bool MetadataStore::lookup(
    std::size_t row_index,
    std::uint64_t &id_out,
    std::string_view &payload_out) const noexcept
{
    if (entries_ == nullptr || row_index >= count_) {
        return false;
    }

    const Entry &e = entries_[row_index];

    id_out = e.id;
    payload_out = std::string_view(
        blob_ + static_cast<std::size_t>(e.payload_offset),
        static_cast<std::size_t>(e.payload_size)
    );

    return true;
}

} // namespace vecstore::storage


