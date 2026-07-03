
#include "volap/core/type.h"

#include <iostream>

namespace {

void check(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << "CHECK failed: " << message << "\n";
        std::exit(1);
    }
}

void test_type_metadata()
{
    using namespace volap::core;

    check(type_v<std::uint8_t> == Type::Bool8, "uint8_t kind");
    check(type_v<std::int64_t> == Type::Int64, "int64_t kind");
    check(type_v<float> == Type::Float32, "float kind");
    check(type_v<double> == Type::Float64, "double kind");
}

} // namespace



int main()
{
    test_type_metadata();
    std::cout << "test_data_chunk: PASS\n";
    return 0;
}
