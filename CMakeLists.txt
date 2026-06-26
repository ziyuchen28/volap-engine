cmake_minimum_required(VERSION 3.16)
project(vecstore LANGUAGES CXX)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(VOLAP_BUILD_BENCH "Build benchmark targets" ON)

set(VOLAP_X86_FAMILY OFF)
# manual check
# unix: uname -m / arch 
# win: echo %PROCESSOR_ARCHITECTURE%
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|X86_64|amd64|AMD64|i[3-6]86)$")
    set(VOLAP_X86_FAMILY ON)
endif()


add_library(volap_kernels STATIC
    src/kernels/sum_product_scalar.cpp
    src/kernels/sum_product.cpp
)
add_library(volap::kernels ALIAS volap_kernels)
target_include_directories(volap_kernels PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(volap_kernels PUBLIC cxx_std_20)
target_compile_options(volap_kernels PRIVATE -Wall -Wextra -Wpedantic -O2)


# ensure compiler vectorization disabled to keep the scalar baseline truly scalar
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
        src/kernels/sum_product_scalar.cpp 
        PROPERTIES
        COMPILE_OPTIONS "-fno-tree-vectorize")
endif()


# if (VOLAP_X86_FAMILY)
#     target_sources(volap_kernels PRIVATE
#         src/kernels/sum_product_x86_avx2_fma.cpp
#     )
#
#     target_compile_definitions(volap_kernels PRIVATE
#         VOLAP_BUILD_X86_AVX2_FMA=1
#     )
#
#     if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#         set_source_files_properties(
#             src/kernels/sum_product_x86_avx2_fma.cpp
#             PROPERTIES
#             COMPILE_OPTIONS "-mavx2;-mfma"
#         )
#     endif()
# endif()


if(PROJECT_IS_TOP_LEVEL)
    option(VOLAP_BUILD_TESTS "Build volap tests" ON)
    option(VOLAP_BUILD_BENCH "Build volap benchmark" ON)
else()
    option(VOLAP_BUILD_TESTS "Build volap tests" OFF)
    option(VOLAP_BUILD_BENCH "Build volap benchmark" OFF)
endif()

# tests 
if(VOLAP_BUILD_TESTS)
  enable_testing()
  add_executable(test_sum_product tests/test_sum_product.cpp)
  target_link_libraries(test_sum_product PRIVATE 
    volap::kernels
  )
  add_test(NAME volap_test_sum_product COMMAND test_sum_product)
endif()


# benchmark
if (VOLAP_BUILD_BENCH)
endif()


