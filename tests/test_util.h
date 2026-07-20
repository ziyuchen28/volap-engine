

#include <cmath>
#include <cstdlib>
#include <iostream>


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
