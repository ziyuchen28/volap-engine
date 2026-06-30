

#include <cmath>
#include <cstdlib>
#include <iostream>


inline void fail(const char *msg)
{
    std::cerr << msg << "\n";
    std::exit(1);
}

inline std::string get_arg(int argc, char **argv, const std::string &key, const std::string &def)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (argv[i] == key) {
            return argv[i + 1];
        }
    }
    return def;
}

inline std::size_t parse_size_arg(int argc, char **argv, const std::string &key, const std::string &def)
{
    const std::string value = get_arg(argc, argv, key, def);
    try {
        return static_cast<std::size_t>(std::stoull(value));
    } catch (const std::exception&) {
        throw std::runtime_error("invalid numeric argument for " + key + ": " + value);
    }
}
