PROJECT_SOURCE_DIR := $(abspath ./)
BUILD_DIR ?= $(PROJECT_SOURCE_DIR)/build
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Linux)
    NUM_JOB := $(shell nproc)
else ifeq ($(UNAME_S), Darwin)
    NUM_JOB := $(shell sysctl -n hw.ncpu)
else
    NUM_JOB := 1
endif

clean:
	rm -rf $(BUILD_DIR)
.PHONY: clean

clang_test_build:
	cmake -S . -B $(BUILD_DIR)/clang -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++" && \
	cmake --build $(BUILD_DIR)/clang --target ciellab_test_11_exceptions_on_rtti_on -j $(NUM_JOB) && \
	cmake --build $(BUILD_DIR)/clang --target ciellab_test_23_exceptions_off_rtti_off -j $(NUM_JOB)

clang_test_run:
	$(BUILD_DIR)/clang/test/ciellab_test_11_exceptions_on_rtti_on && $(BUILD_DIR)/clang/test/ciellab_test_23_exceptions_off_rtti_off

clang_test: clang_test_build clang_test_run

gcc_test_build:
	cmake -S . -B $(BUILD_DIR)/gcc -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ && \
	cmake --build $(BUILD_DIR)/gcc --target ciellab_test_11_exceptions_on_rtti_on -j $(NUM_JOB) && \
	cmake --build $(BUILD_DIR)/gcc --target ciellab_test_23_exceptions_off_rtti_off -j $(NUM_JOB)

gcc_test_run:
	$(BUILD_DIR)/gcc/test/ciellab_test_11_exceptions_on_rtti_on && $(BUILD_DIR)/gcc/test/ciellab_test_23_exceptions_off_rtti_off

gcc_test: gcc_test_build gcc_test_run

test: clang_test_build gcc_test_build clang_test_run gcc_test_run
.PHONY: test

clang_benchmark_build:
	cmake -S . -B $(BUILD_DIR)/clang -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++" && \
    cmake --build $(BUILD_DIR)/clang --target ciellab_benchmark -j $(NUM_JOB)

clang_benchmark_run:
	$(BUILD_DIR)/clang/benchmark/ciellab_benchmark

clang_benchmark: clang_benchmark_build clang_benchmark_run

gcc_benchmark_build:
	cmake -S . -B $(BUILD_DIR)/gcc -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ && \
    cmake --build $(BUILD_DIR)/gcc --target ciellab_benchmark -j $(NUM_JOB)

gcc_benchmark_run:
	$(BUILD_DIR)/gcc/benchmark/ciellab_benchmark

gcc_benchmark: gcc_benchmark_build gcc_benchmark_run

benchmark: clang_benchmark_build gcc_benchmark_build clang_benchmark_run gcc_benchmark_run
.PHONY: benchmark

format:
	./format.sh run
.PHONY: format

check_format:
	./format.sh check
.PHONY: check_format
