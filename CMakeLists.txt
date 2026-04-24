cmake_minimum_required(VERSION 3.16)
project(vecstore LANGUAGES CXX)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(VECSTORE_BUILD_BENCH "Build benchmark targets" ON)

set(VECSTORE_X86_FAMILY OFF)
# manual check
# unix: uname -m / arch 
# win: echo %PROCESSOR_ARCHITECTURE%
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|X86_64|amd64|AMD64|i[3-6]86)$")
    set(VECSTORE_X86_FAMILY ON)
endif()


# vector math
add_library(vecstore_vecmath STATIC
    src/vecmath/dot_scalar.cpp
    src/vecmath/dot_dispatch.cpp
    src/vecmath/normalize.cpp
)
add_library(vecstore::vecmath ALIAS vecstore_vecmath)
target_include_directories(vecstore_vecmath PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(vecstore_vecmath PUBLIC cxx_std_20)
target_compile_options(vecstore_vecmath PRIVATE -Wall -Wextra -Wpedantic -O2)


# ensure compiler vectorization disabled to keep the scalar baseline truly scalar
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
        src/vecmath/dot_scalar.cpp 
        PROPERTIES
        COMPILE_OPTIONS "-fno-tree-vectorize")
endif()


if (VECSTORE_X86_FAMILY)
    target_sources(vecstore_vecmath PRIVATE
        src/vecmath/dot_x86_avx2.cpp
    )

    # -D flag
    target_compile_definitions(vecstore_vecmath PRIVATE
        VECSTORE_BUILD_X86_AVX2=1
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set_source_files_properties(
            src/vecmath/dot_x86_avx2.cpp
            PROPERTIES
            COMPILE_OPTIONS "-mavx2;-mfma"
        )
    endif()
endif()


# flat search 
add_library(vecstore_flat STATIC
    src/flat/search.cpp
)
add_library(vecstore::flat ALIAS vecstore_flat)

target_include_directories(vecstore_flat PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(vecstore_flat PUBLIC cxx_std_20)
target_compile_options(vecstore_flat PRIVATE -Wall -Wextra -Wpedantic -O2)

target_link_libraries(vecstore_flat PUBLIC
    vecstore::vecmath
)



# storage 
add_library(vecstore_storage STATIC
    src/storage/dense_vector_store.cpp
    src/storage/metadata_store.cpp
)
add_library(vecstore::storage ALIAS vecstore_storage)

target_include_directories(vecstore_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(vecstore_storage PUBLIC cxx_std_20)
target_compile_options(vecstore_storage PRIVATE -Wall -Wextra -Wpedantic -O2)



# tests 
enable_testing()

add_executable(test_dot
    tests/test_dot.cpp
)
target_link_libraries(test_dot PRIVATE 
    vecstore::vecmath
)
add_test(NAME vecstore_test_dot COMMAND test_dot)


add_executable(test_search_flat
    tests/test_search_flat.cpp
)
target_link_libraries(test_search_flat PRIVATE 
    vecstore::flat
)
add_test(NAME vecstore_test_search_flat COMMAND test_search_flat)


add_executable(test_dense_vector_store
    tests/test_dense_vector_store.cpp
)
target_link_libraries(test_dense_vector_store PRIVATE 
    vecstore::storage
    vecstore::flat
)
add_test(NAME vecstore_test_dense_vector_store COMMAND test_dense_vector_store)



add_executable(test_metadata_store
    tests/test_metadata_store.cpp
)
target_link_libraries(test_metadata_store PRIVATE 
    vecstore::storage
    vecstore::flat
)
add_test(NAME vecstore_test_metadata_store COMMAND test_metadata_store)


# benchmark
if (VECSTORE_BUILD_BENCH)

    add_executable(bench_dot
        bench/bench_dot.cpp
    )
    target_link_libraries(bench_dot 
        PRIVATE 
        vecstore::vecmath
    )

    add_executable(bench_search_flat
        bench/bench_search_flat.cpp
    )
    target_link_libraries(bench_search_flat 
        PRIVATE 
        vecstore::flat
        vecstore::vecmath
        vecstore::storage
    )

endif()


