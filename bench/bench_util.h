
#pragma once

#include <string>

inline std::string get_arg(int argc,
                           char **argv,
                           const std::string &key,
                           const std::string &default_value)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }

    return default_value;
}

#if defined(__GNUC__) || defined(__clang__)
template <typename T>
inline void do_not_optimize(const T &value)
{
    __asm__ __volatile__("" 
                         : 
                         : "g"(value) 
                         : "memory");
}
#else
template <typename T>
inline void do_not_optimize(const T &value)
{
    volatile T sink = value;
    (void)sink;
}
#endif

