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

COMPILER_PATH ?=
COMPILER_FLAGS ?=

clean:
	rm -rf $(BUILD_DIR) && rm -rf third_party/*build
.PHONY: clean

test:
	cmake -S . -B $(BUILD_DIR) -G Ninja $(if $(COMPILER_PATH),-DCMAKE_CXX_COMPILER=$(COMPILER_PATH)) $(if $(COMPILER_FLAGS),-DCMAKE_CXX_FLAGS="$(COMPILER_FLAGS)")
	cmake --build $(BUILD_DIR) --target ciellab_test_11_exceptions_on_rtti_on -j $(NUM_JOB)
	cmake --build $(BUILD_DIR) --target ciellab_test_20_exceptions_off_rtti_off -j $(NUM_JOB)
	$(BUILD_DIR)/test/ciellab_test_11_exceptions_on_rtti_on
	$(BUILD_DIR)/test/ciellab_test_20_exceptions_off_rtti_off
.PHONY: test

benchmark:
	cmake -S . -B $(BUILD_DIR) -G Ninja $(if $(COMPILER_PATH),-DCMAKE_CXX_COMPILER=$(COMPILER_PATH)) $(if $(COMPILER_FLAGS),-DCMAKE_CXX_FLAGS="$(COMPILER_FLAGS)")
	cmake --build $(BUILD_DIR) --target ciellab_benchmark -j $(NUM_JOB)
	$(BUILD_DIR)/benchmark/ciellab_benchmark
.PHONY: benchmark

format:
	./format.sh run $(PROJECT_SOURCE_DIR)/include $(PROJECT_SOURCE_DIR)/test/src $(PROJECT_SOURCE_DIR)/benchmark/src
.PHONY: format

check_format:
	./format.sh check $(PROJECT_SOURCE_DIR)/include $(PROJECT_SOURCE_DIR)/test/src $(PROJECT_SOURCE_DIR)/benchmark/src
.PHONY: check_format
