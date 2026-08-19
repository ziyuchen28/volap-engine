
#include "volap/core/flat_vector.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

#include <algorithm>
#include <initializer_list>

using namespace volap::core;

inline void fail(const char *msg)
{
    std::cerr << msg << "\n";
    std::exit(1);
}

inline void validate(bool condition, const char *err_message)
{
    if (!condition) {
        std::cerr << "Validation failed: " << err_message << '\n';
        std::exit(1);
    }
}

template <typename Exception, typename Function>
inline void validate_throws(Function &&function, const char *message)
{
    bool threw = false;
    try {
        function();
    } catch (const Exception&) {
        threw = true;
    }
    validate(threw, message);
}

inline void succeeded(std::string_view func_name)
{
    std::cout << "[PASS] " << func_name << std::endl;
}

template <typename T>
inline Vector make_flat_vector(std::initializer_list<T> values)
{
    Vector vector = FlatVector::create(type_v<T>, values.size());

    T *data = FlatVector::get_mutable_data<T>(vector);
    std::copy(values.begin(), values.end(), data);

    vector.set_size(values.size());

    return vector;
}

