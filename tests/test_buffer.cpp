
#include "volap/core/buffer.h"

#include "test_util.h"

#include <vector>

namespace 
{

using namespace volap::core;

void test_empty_buffer()
{
    Buffer buffer;
    validate(buffer.empty(), "default buffer is empty");
    validate(buffer.type() == BufferType::Empty, "empty buffer");
    validate(buffer.data() == nullptr, "data is null");
    validate(buffer.mut_data() == nullptr, "data is null");
    validate(buffer.size_bytes() == 0, "empty size");
    validate(!buffer.is_exclusive(), "buffer is not unique");
    validate(!buffer.is_writable(), "buffer is not writable");
    std::cout << "[PASS] test_empty_buffer\n";
}

void test_owned_buffer_alignment()
{
    constexpr std::size_t bytes = 1024;
    constexpr std::size_t alignment = 64;

    auto buffer = Buffer::allocate(bytes, alignment);

    validate(!buffer.empty(), "owned buffer is non-empty");
    validate(buffer.type() == BufferType::Owned, "owned buffer type");
    validate(buffer.alignment() == alignment, "owned buffer alignment");
    validate(buffer.size_bytes() == bytes, "owned buffer size");
    validate(buffer.is_exclusive(), "owned buffer starts exclusive");
    validate(buffer.is_writable(), "exclusive owned buffer is writable");

    const auto address = reinterpret_cast<std::uintptr_t>(buffer.data());

    validate(address % alignment == 0, "owned data has valid alignment");

    std::byte *writable = buffer.mut_data();
    writable[0] = std::byte{0x01};
    writable[bytes - 1] = std::byte{0x02};

    validate(buffer.data()[0] == std::byte{0x01}, "first byte");
    validate(buffer.data()[bytes - 1] == std::byte{0x02}, "last byte");
    std::cout << "[PASS] test_owned_buffer_alignment\n";
}

void test_owned_buffer_write()
{
    constexpr std::size_t bytes = 1024;
    constexpr std::size_t alignment = 64;

    auto buffer = Buffer::allocate(bytes, alignment);

    validate(!buffer.empty(), "owned buffer is non-empty");
    validate(buffer.type() == BufferType::Owned, "owned buffer type");
    validate(buffer.alignment() == alignment, "owned buffer alignment");
    validate(buffer.size_bytes() == bytes, "owned buffer size");
    validate(buffer.is_exclusive(), "owned buffer starts exclusive");
    validate(buffer.is_writable(), "exclusive owned buffer is writable");

    const auto address = reinterpret_cast<std::uintptr_t>(buffer.data());

    validate(address % alignment == 0, "owned data has valid alignment");

    std::byte *writable = buffer.mut_data();
    writable[0] = std::byte{0x01};
    writable[bytes - 1] = std::byte{0x02};

    validate(buffer.data()[0] == std::byte{0x01}, "first byte");
    validate(buffer.data()[bytes - 1] == std::byte{0x02}, "last byte");
    std::cout << "[PASS] test_owned_buffer_write\n";
}

void test_copy_buffer_shares_storage()
{
    auto original = Buffer::allocate(128);

    validate(original.is_exclusive(), "original buffer exclusive");
    validate(original.is_writable(), "original buffer writable");

    Buffer copy = original;

    validate(original.data() == copy.data(), "copy buffer shares data pointer");
    validate(!original.is_exclusive(), "original buffer is now shared");
    validate(!copy.is_exclusive(), "copy buffer is shared");
    validate(!original.is_writable(), "original buffer is not writable");
    validate(!copy.is_writable(), "copy buffer is not writable");

    copy = volap::core::Buffer{};

    validate(original.is_exclusive(), "original buffer exclusive after copy reassigned");
    validate(original.is_writable(), "original buffer writable after copy reassigned");

    original.mut_data()[0] = std::byte{0x01};

    validate(original.data()[0] == std::byte{0x01},
            "original writable after copy release");
    std::cout << "[PASS] test_copy_buffer_shares_storage\n";
}

void test_move_transfers_handle()
{
    auto source = Buffer::allocate(64);
    const std::byte *original_data = source.data();

    Buffer destination = std::move(source);

    validate(source.empty(), "source buffer is empty");
    validate(destination.data() == original_data, "move preserves data");
    validate(destination.is_owned(), "moved destination remains owned");
    validate(destination.is_exclusive(), "moved destination is exclusive");
    std::cout << "[PASS] test_empty_buffer\n";
}

void test_external_buffer_is_read_only()
{
    auto owner =
        std::make_shared<std::vector<std::int64_t>>(
            std::initializer_list<std::int64_t>{10, 20, 30});

    Buffer buffer =
        Buffer::wrap_external(owner->data(),
                              owner->size() * sizeof(std::int64_t),
                              owner);

    validate(buffer.type() == BufferType::External, "external buffer");
    validate(buffer.is_exclusive(), "external buffer starts as exclusive");
    validate(!buffer.is_writable(), "external buffer is read-only");

    const auto *readable =
        reinterpret_cast<const std::int64_t*>(buffer.data());

    validate(readable[0] == 10, "external buffer value 0");
    validate(readable[1] == 20, "external buffer value 1");
    validate(readable[2] == 30, "external buffer value 2");
    std::cout << "[PASS] test_move_transfers_handle\n";
}


void test_pinning_external_buffer_lifetime()
{
    std::weak_ptr<std::vector<std::int64_t>> observer;
    
    Buffer buffer;

    {
        auto owner =
            std::make_shared<std::vector<std::int64_t>>(
                std::initializer_list<std::int64_t>{100, 200, 300});

        observer = owner;

        buffer = Buffer::wrap_external(owner->data(),
                                       owner->size() * sizeof(std::int64_t),
                                       owner
        );
    // owner destroyed 
    }

    // The local `owner` shared_ptr has been destroyed, but Buffer's
    // lifetime token keeps the vector allocation alive.
    validate(!observer.expired(),
            "external owner remains alive while buffer exists");

    const auto* values = reinterpret_cast<const std::int64_t*>(buffer.data());

    validate(values[0] == 100, "pinned external buffer value 0");
    validate(values[2] == 300, "pinned external buffer value 2");

    // original buffer destroyed
    buffer = Buffer{};

    validate(observer.expired(), "external owner released");
    std::cout << "[PASS] test_pinning_external_buffer_lifetime\n";
}

} // namespace 

int main()
{
    test_empty_buffer();
    test_owned_buffer_alignment();
    test_owned_buffer_write();
    test_copy_buffer_shares_storage();
    test_move_transfers_handle();
    test_external_buffer_is_read_only();
    test_pinning_external_buffer_lifetime();
    std::cout << "[DONE] test_buffer\n";
}
