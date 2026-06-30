BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= RelWithDebInfo

# auto, avx2, scalar
KERNEL_IMPL ?= auto

.PHONY: all configure build test test-verbose bench-dot bench-search-flat clean

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR) -j -- --no-print-directory

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

test-verbose: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure -V


ROWS ?= 1048576
ITERS ?= 1000
WARMUP ?= 20

bench-sum-product: build
	./$(BUILD_DIR)/bench_sum_product --rows $(ROWS) --iters $(ITERS) --warmup $(WARMUP)

clean:
	rm -rf $(BUILD_DIR)
