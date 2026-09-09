
#include "benchmark_util.h"


#include <iostream>
#include <fstream>
#include <thread>

#if defined(__linux__)
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

#elif defined(__APPLE__)
#include <sys/utsname.h>
#include <unistd.h>

#elif defined(_WIN32)
#include <windows.h>

#endif

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

#ifndef BUILD_TYPE
#define BUILD_TYPE "unknown"
#endif

#define SUCCESS 0


// TODO: fully support windows 
namespace volap::bench 
{

namespace 
{

std::string architecture_string()
{
#if defined(__x86_64__)    || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#else
    return "unknown";
#endif
}

std::string cpu_model_string()
{
#if defined(__linux__)
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo) {
        return "unknown";
    }

    std::string line;
    while (std::getline(cpuinfo, line)) {
        const char *prefix = "model name";

        // faster than .find(prefix)
        if (line.rfind(prefix, 0) != 0) {
            continue;
        }

        const std::size_t separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        std::size_t start = separator + 1;
        while (start < line.size() && line[start] == ' ') {
            ++start;
        }

        return line.substr(start);
    }

    return "unknown";


#elif defined(__APPLE__)
    // TODO
    return "unknown";

#elif defined(_WIN32)
    // TODO
    return "unknown";

#endif
}


std::size_t logical_cpu_count()
{


#if defined(__linux__)
    const long count = sysconf(_SC_NPROCESSORS_ONLN);

    if (count > 0) {
        return static_cast<std::size_t>(count);
    }


#elif defined(__APPLE__)
    std::uint32_t count = 0;
    std::size_t size = sizeof(count);

    if (sysctlbyname("hw.logicalcpu", &count, &size, nullptr, 0) == 0) 
    {
        return static_cast<std::size_t>(count);
    }

#endif

    return static_cast<std::size_t>(
        std::thread::hardware_concurrency());
}

std::uint64_t total_memory_bytes()
{
#if defined(__linux__)
    struct sysinfo info {};
    if (sysinfo(&info) != 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(info.totalram) *
           // this is just bytes per page
           static_cast<std::uint64_t>(info.mem_unit);

#elif defined(__APPLE__)
    // TODO use sysctl / sysctlbyname("hw.memsize")
    return 0;

#elif defined(_WIN32)
    // TODO use GlobalMemoryStatusEx()
    return 0;

#else
    return 0;
#endif
}

std::string operating_system_string()
{
// TODO: get win's release version
#if defined(_WIN32)
    return "Windows";
#endif

    struct utsname info {};
    if (uname(&info) != SUCCESS) 
    {
#if defined(__APPLE__)
        return "macOS";
#elif defined(__linux__)
        return "Linux";
#else
        return "unknown";
#endif
    }

// Additonal info available
#if defined(__APPLE__)
    std::string result = "macOS";
#elif defined(__linux__)
    std::string result = "Linux";
#else
    std::string result = info.sysname;
#endif
    result += " ";
    result += info.release;
    return result;

}

std::string compiler_string()
{
#if defined(__clang__)
#if defined(__apple_build_version__)
    return "AppleClang " __clang_version__;
#else
    return "Clang " __clang_version__;
#endif
#elif defined(__GNUC__)
    return "GCC " __VERSION__;
#else
    return "unknown";
#endif
}

double bytes_to_gib(std::uint64_t bytes)
{
    constexpr double bytes_per_gib =
        1024.0 * 1024.0 * 1024.0;
    return static_cast<double>(bytes) / bytes_per_gib;
}

} // anonymous namespace



void print_benchmark_environment()
{

    BenchmarkEnvironment environment;

    environment.commit = GIT_COMMIT;
    environment.build_type = BUILD_TYPE;

    environment.arch = architecture_string();
    environment.cpu_model = cpu_model_string();
    environment.logical_cpus = logical_cpu_count();
    environment.memory_bytes = total_memory_bytes();

    environment.os = operating_system_string();
    environment.compiler = compiler_string();

    std::cout << "commit: " << environment.commit << '\n';
    std::cout << "build_type: " << environment.build_type << '\n';

    std::cout << "arch: " << environment.arch << '\n';
    std::cout << "cpu: " << environment.cpu_model << '\n';
    std::cout << "logical_cpus: " << environment.logical_cpus << '\n';

    if (environment.memory_bytes != 0) {
        std::cout << "memory_gib: "
                  << bytes_to_gib(environment.memory_bytes)
                  << '\n';
    } 
    else {
        std::cout << "memory_gib: unknown\n";
    }

    std::cout << "os: " << environment.os << '\n';
    std::cout << "compiler: " << environment.compiler << '\n';
    std::cout << '\n';

}

} // namespace volap::bench
