BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= RelWithDebInfo

# auto, avx2, scalar
KERNEL_IMPL ?= auto
SEARCH_STORAGE ?= mmap
SEARCH_METRIC ?= cosine


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

bench-dot: build
	./$(BUILD_DIR)/bench_dot --dim 1536 --iters 10000000 --warmup 10000 --impl $(KERNEL_IMPL)

# bench-search-flat: build
# 	./$(BUILD_DIR)/bench_search_flat --count 20000 --dim 1536 --k 10 --iters 100 --warmup 3 --impl $(DOT_IMPL)

bench-search-flat: build
	./$(BUILD_DIR)/bench_search_flat \
		--count 20000 \
		--dim 1536 \
		--k 10 \
		--query_count 1 \
		--iters 100 \
		--warmup 10 \
		--storage $(SEARCH_STORAGE) \
		--search-metric $(SEARCH_METRIC) \
		--kernel-impl $(KERNEL_IMPL) \
		--path /tmp/vecstore_flat_search.bin

clean:
	rm -rf $(BUILD_DIR)
