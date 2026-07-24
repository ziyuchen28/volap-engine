
#pragma once

#include <memory>

namespace volap::core 
{

// Memory alignment defaulted to 64 to match cache line bandwidth.
// TO DO: 4kb once DMA involved? 2MB once huge page involved?
// Or use a different kind of buffer with diff alignmemnt rules
// for example: DirectIoBuffer and HugePageBuffer
#define ALIGNMENT_DEFAULT 64

enum class BufferType 
{
    Empty,
    Owned,
    External
};


class Buffer 
{
public:
    // A shared pointer to wrap the external data extending it's lifetime
    // const void: we don't care what the data is
    using BufferLifetime = std::shared_ptr<const void>;

    Buffer() noexcept = default;

    ~Buffer();

    // Buffer copies share the same underlying storage.
    Buffer(const Buffer&) noexcept; 

    Buffer& operator=(const Buffer&) noexcept;

    Buffer(Buffer&&) noexcept;

    Buffer& operator=(Buffer&&) noexcept;

    // Allocates uninitialized, writable, aligned memory.
    static Buffer allocate(std::size_t bytes,
                           std::size_t alignment = ALIGNMENT_DEFAULT);

    // Creates a read-only view over externally owned memory.
    // lifetime must keep the external allocation alive for as long as
    // any Buffer referencing it exists.
    static Buffer wrap_external(const void* data,
                                std::size_t bytes,
                                BufferLifetime lifetime,
                                std::size_t alignment = 1);

    const std::byte *data() const noexcept;

    // Returns mutable memory only when:
    //   1. the buffer owns the allocated data
    //   2. no other buffer currently shares the same storage.
    std::byte *mut_data();

    std::size_t size_bytes() const noexcept;

    std::size_t alignment() const noexcept;

    BufferType type() const noexcept;

    bool empty() const noexcept;

    bool is_owned() const noexcept;

    bool is_external() const noexcept;

    // True when this is the only buffer referring to the storage.
    bool is_exclusive() const noexcept;

    // mutability check
    bool is_writable() const noexcept;

private:
    struct BufferStorage;

    // explicit Buffer(std::shared_ptr<BufferStorage> storage) noexcept;
    explicit Buffer(const std::shared_ptr<BufferStorage> &storage) noexcept;
    explicit Buffer(std::shared_ptr<BufferStorage> &&storage) noexcept;
    std::shared_ptr<BufferStorage> storage_;
};

} // namespace volap::core


