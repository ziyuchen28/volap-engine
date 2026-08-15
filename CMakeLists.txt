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

# core data structure
add_library(volap_core STATIC
  src/core/buffer.cpp
  src/core/vector.cpp
  src/core/flat_vector.cpp
  src/core/data_chunk.cpp
)
add_library(volap::core ALIAS volap_core)
target_include_directories(volap_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(volap_core PUBLIC cxx_std_20)
target_compile_options(volap_core PRIVATE -Wall -Wextra -Wpedantic -O2)

# math kernel
add_library(volap_kernels STATIC
  src/kernels/sum_product_scalar.cpp
  src/kernels/sum_product.cpp
  src/kernels/select.cpp
)
add_library(volap::kernels ALIAS volap_kernels)
target_include_directories(volap_kernels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(volap_kernels PUBLIC cxx_std_20)
target_compile_options(volap_kernels PRIVATE -Wall -Wextra -Wpedantic -O2)
target_link_libraries(volap_kernels PUBLIC volap::core)


# ensure compiler vectorization disabled to keep the scalar baseline truly scalar
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set_source_files_properties(
    src/kernels/sum_product_scalar.cpp 
    PROPERTIES
    COMPILE_OPTIONS "-fno-tree-vectorize")
endif()


if (VOLAP_X86_FAMILY)
  target_sources(volap_kernels PRIVATE
    src/kernels/sum_product_x86_avx2_fma.cpp
  )

  target_compile_definitions(volap_kernels PRIVATE
    VOLAP_BUILD_X86_AVX2_FMA=1
  )

  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
      src/kernels/sum_product_x86_avx2_fma.cpp
      PROPERTIES
      COMPILE_OPTIONS "-mavx2;-mfma"
    )
  endif()
endif()


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

  add_executable(test_buffer tests/test_buffer.cpp)
  target_link_libraries(test_buffer PRIVATE 
    volap::core
  )
  add_test(NAME volap_test_buffer COMMAND test_buffer)

  add_executable(test_vector tests/test_vector.cpp)
  target_link_libraries(test_vector PRIVATE 
    volap::core
  )
  add_test(NAME volap_test_vector COMMAND test_vector)

  add_executable(test_data_chunk tests/test_data_chunk.cpp)
  target_link_libraries(test_data_chunk PRIVATE 
    volap::core
  )
  add_test(NAME volap_test_data_chunk COMMAND test_data_chunk)

  add_executable(test_select tests/test_select.cpp)
  target_link_libraries(test_select PRIVATE volap::kernels)
  add_test(NAME volap_test_select COMMAND test_select)

endif()


# benchmark
if (VOLAP_BUILD_BENCH)
  add_executable(bench_sum_product
    bench/bench_sum_product.cpp
  )

  target_link_libraries(bench_sum_product
    PRIVATE
      volap::kernels
  )
endif()


