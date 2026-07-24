#include "volap/core/buffer.h"

#include <cstddef>
#include <bit>

namespace volap::core 
{

namespace 
{

bool is_power_of_two(std::size_t alignment) noexcept
{
    return alignment != 0 && std::has_single_bit(alignment);
}

void validate_alignment(std::size_t alignment)
{
    if (!is_power_of_two(alignment)) {
        throw std::invalid_argument(
            "Buffer allocation alignment must be non-zero power of two"
        );
    }

    // Alignment smaller than std::max_align_t not supported by over aligned new/delete.
    if (alignment < alignof(std::max_align_t)) {
        throw std::invalid_argument(
            "Buffer alignment must be at least alignof(std::max_align_t)"
        );
    }
}

} // namespace

struct Buffer::BufferStorage 
{
    BufferType type = BufferType::Empty;
    std::size_t size_bytes = 0;
    std::size_t alignment = 1;

    const std::byte *data = nullptr;
    std::byte *mut_data = nullptr;

    // Used only to keep the externally owned data alive.
    // Doesn't care what the data is
    BufferLifetime lifetime;

    ~BufferStorage()
    {
        if (type == BufferType::Owned && mut_data != nullptr) {
            ::operator delete(
                mut_data,
                std::align_val_t{alignment}
            );
        }
    }
};

// Buffer::Buffer(std::shared_ptr<BufferStorage> storage) noexcept
//     : storage_(std::move(storage))
// {}

Buffer::Buffer(const std::shared_ptr<BufferStorage> &storage) noexcept 
    : storage_(storage) 
{}

Buffer::Buffer(std::shared_ptr<BufferStorage> &&storage) noexcept 
    : storage_(std::move(storage)) 
{}


Buffer::~Buffer() = default;

Buffer::Buffer(const Buffer&) noexcept = default;
Buffer& Buffer::operator=(const Buffer&) noexcept = default;

Buffer::Buffer(Buffer&&) noexcept = default;
Buffer& Buffer::operator=(Buffer&&) noexcept = default;

Buffer Buffer::allocate(std::size_t bytes,
                        std::size_t alignment)
{
    validate_alignment(alignment);
    if (bytes == 0) {
        return {};
    }

    auto storage = std::make_shared<BufferStorage>();
    auto *data = static_cast<std::byte*>(
        ::operator new(bytes, std::align_val_t{alignment}));

    storage->type = BufferType::Owned;
    storage->size_bytes = bytes;
    storage->alignment = alignment;
    storage->data = data;
    storage->mut_data = data;

    return Buffer(std::move(storage));
}

Buffer Buffer::wrap_external(const void *data,
                             std::size_t bytes,
                             BufferLifetime lifetime,
                             std::size_t alignment)
{
    // TO DO: should we handle alignment if data not created via buffer allocator?
    // for SIMD: uses unaligned instruction for now
    (void)alignment;
    if (bytes == 0) {
        return {};
    }
    if (data == nullptr) {
        throw std::invalid_argument(
            "External buffer has null data");
    }
    if (!lifetime) {
        throw std::invalid_argument(
            "Non-empty external buffer requires a lifetime token");
    }

    auto storage = std::make_shared<BufferStorage>();

    storage->type = BufferType::External;
    storage->data = static_cast<const std::byte*>(data);
    storage->mut_data = nullptr;
    storage->size_bytes = bytes;
    storage->alignment = 1;
    storage->lifetime = std::move(lifetime);

    return Buffer(std::move(storage));
}

const std::byte *Buffer::data() const noexcept
{
    if (storage_) return storage_->data;
    return nullptr;
}

std::byte *Buffer::mut_data()
{
    if (!storage_) {
        return nullptr;
    }

    if (storage_->type != BufferType::Owned) {
        throw std::logic_error(
            "External buffer is read-only"
        );
    }

    if (storage_.use_count() != 1) {
        throw std::logic_error(
            "Shared buffer is not writable"
        );
    }

    return storage_->mut_data;
}

std::size_t Buffer::size_bytes() const noexcept
{
    if (storage_) return storage_->size_bytes;
    return 0;
}

std::size_t Buffer::alignment() const noexcept
{
    if (storage_) return storage_->alignment;
    return 1;
}

BufferType Buffer::type() const noexcept
{
    if (storage_) return storage_->type;
    return BufferType::Empty;
}

bool Buffer::is_owned() const noexcept
{
    if (!storage_) return false;
    return storage_->type == BufferType::Owned;
}

bool Buffer::is_external() const noexcept
{
    if (!storage_) return false;
    return storage_->type == BufferType::External;
}

bool Buffer::empty() const noexcept
{
    return size_bytes() == 0;
}

bool Buffer::is_exclusive() const noexcept
{
    if (!storage_) return false;
    return storage_.use_count() == 1;
}

bool Buffer::is_writable() const noexcept
{
    if (!storage_) return false;
    return storage_->type == BufferType::Owned && storage_.use_count() == 1;
}

} // namespace volap::core


