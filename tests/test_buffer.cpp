
#include "volap/core/buffer.h"

#include "test_helper.h"

#include <vector>

namespace 
{

using namespace volap::core;

void test_empty_buffer()
{
    Buffer buffer;
    check(buffer.empty(), "default buffer is empty");
    check(buffer.type() == BufferType::Empty, "empty buffer");
    check(buffer.data() == nullptr, "data is null");
    check(buffer.mut_data() == nullptr, "data is null");
    check(buffer.size_bytes() == 0, "empty size");
    check(!buffer.is_exclusive(), "buffer is not unique");
    check(!buffer.is_writable(), "buffer is not writable");
}

void test_owned_buffer_alignment_and_write()
{
    constexpr std::size_t bytes = 1024;
    constexpr std::size_t alignment = 64;

    auto buffer = Buffer::allocate(bytes, alignment);

    check(!buffer.empty(), "owned buffer is non-empty");
    check(buffer.type() == BufferType::Owned, "owned buffer type");
    check(buffer.alignment() == alignment, "owned buffer alignment");
    check(buffer.size_bytes() == bytes, "owned buffer size");
    check(buffer.is_exclusive(), "owned buffer starts exclusive");
    check(buffer.is_writable(), "exclusive owned buffer is writable");

    const auto address =
        reinterpret_cast<std::uintptr_t>(buffer.data());

    check(address % alignment == 0,
          "owned data has valid alignment");

    std::byte *writable = buffer.mut_data();
    writable[0] = std::byte{0x01};
    writable[bytes - 1] = std::byte{0x02};

    check(buffer.data()[0] == std::byte{0x01}, "first byte");
    check(buffer.data()[bytes - 1] == std::byte{0x02}, "last byte");
}

void test_external_buffer_is_read_only()
{
    auto owner =
        std::make_shared<std::vector<std::int64_t>>(
            std::initializer_list<std::int64_t>{10, 20, 30});

    Buffer buffer =
        volap::core::Buffer::wrap_external(
            owner->data(),
            owner->size() * sizeof(std::int64_t),
            owner
        );

    check(buffer.type() == BufferType::External, "external buffer");
    check(buffer.is_exclusive(), "external buffer starts as exclusive");
    check(!buffer.is_writable(), "external buffer is read-only");

    const auto *readable =
        reinterpret_cast<const std::int64_t*>(buffer.data());

    check(readable[0] == 10, "external value 0");
    check(readable[1] == 20, "external value 1");
    check(readable[2] == 30, "external value 2");

}

} // namespace 

int main()
{
    test_empty_buffer();
}
