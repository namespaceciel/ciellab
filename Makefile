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

GCC_PATH ?= gcc
GCC_PATH := $(or $(CIELLAB_GCC),$(GCC_PATH))
GXX_PATH ?= g++
GXX_PATH := $(or $(CIELLAB_GXX),$(GXX_PATH))

CLANG_PATH ?= clang
CLANG_PATH := $(or $(CIELLAB_CLANG),$(CLANG_PATH))
CLANGXX_PATH ?= clang++
CLANGXX_PATH := $(or $(CIELLAB_CLANGXX),$(CLANGXX_PATH))

clean:
	rm -rf $(BUILD_DIR)
.PHONY: clean

# -DCMAKE_CXX_CLANG_TIDY="clang-tidy"

clang_test_build:
	cmake -S . -B $(BUILD_DIR)/clang -DCMAKE_C_COMPILER=$(CLANG_PATH) -DCMAKE_CXX_COMPILER=$(CLANGXX_PATH) -DCMAKE_CXX_FLAGS="-stdlib=libc++" && \
	cmake --build $(BUILD_DIR)/clang --target ciellab_test_11_exceptions_on_rtti_on -j $(NUM_JOB) && \
	cmake --build $(BUILD_DIR)/clang --target ciellab_test_20_exceptions_off_rtti_off -j $(NUM_JOB)

clang_test_run:
	$(BUILD_DIR)/clang/test/ciellab_test_11_exceptions_on_rtti_on && $(BUILD_DIR)/clang/test/ciellab_test_20_exceptions_off_rtti_off

clang_test: clang_test_build clang_test_run

gcc_test_build:
	cmake -S . -B $(BUILD_DIR)/gcc -DCMAKE_C_COMPILER=$(GCC_PATH) -DCMAKE_CXX_COMPILER=$(GXX_PATH) && \
	cmake --build $(BUILD_DIR)/gcc --target ciellab_test_11_exceptions_on_rtti_on -j $(NUM_JOB) && \
	cmake --build $(BUILD_DIR)/gcc --target ciellab_test_20_exceptions_off_rtti_off -j $(NUM_JOB)

gcc_test_run:
	$(BUILD_DIR)/gcc/test/ciellab_test_11_exceptions_on_rtti_on && $(BUILD_DIR)/gcc/test/ciellab_test_20_exceptions_off_rtti_off

gcc_test: gcc_test_build gcc_test_run

test: clang_test_build gcc_test_build clang_test_run gcc_test_run
.PHONY: test

clang_benchmark_build:
	cmake -S . -B $(BUILD_DIR)/clang -DCMAKE_C_COMPILER=$(CLANG_PATH) -DCMAKE_CXX_COMPILER=$(CLANGXX_PATH) -DCMAKE_CXX_FLAGS="-stdlib=libc++" && \
    cmake --build $(BUILD_DIR)/clang --target ciellab_benchmark -j $(NUM_JOB)

clang_benchmark_run:
	$(BUILD_DIR)/clang/benchmark/ciellab_benchmark

clang_benchmark: clang_benchmark_build clang_benchmark_run

gcc_benchmark_build:
	cmake -S . -B $(BUILD_DIR)/gcc -DCMAKE_C_COMPILER=$(GCC_PATH) -DCMAKE_CXX_COMPILER=$(GXX_PATH) && \
    cmake --build $(BUILD_DIR)/gcc --target ciellab_benchmark -j $(NUM_JOB)

gcc_benchmark_run:
	$(BUILD_DIR)/gcc/benchmark/ciellab_benchmark

gcc_benchmark: gcc_benchmark_build gcc_benchmark_run

benchmark: clang_benchmark_build gcc_benchmark_build clang_benchmark_run gcc_benchmark_run
.PHONY: benchmark

format:
	./format.sh run $(PROJECT_SOURCE_DIR)/include $(PROJECT_SOURCE_DIR)/test/src $(PROJECT_SOURCE_DIR)/benchmark/src
.PHONY: format

check_format:
	./format.sh check $(PROJECT_SOURCE_DIR)/include $(PROJECT_SOURCE_DIR)/test/src $(PROJECT_SOURCE_DIR)/benchmark/src
.PHONY: check_format

cc:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_EXPORT_COMPILE_COMMANDS=1
.PHONY: cc
