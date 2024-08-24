PROJECT_SOURCE_DIR := $(abspath ./)
BUILD_DIR ?= $(PROJECT_SOURCE_DIR)/build
UNAME_S := $(uname -s)

ifeq ($(UNAME_S), Linux)
    NUM_JOB := $(nproc)
else ifeq ($(UNAME_S), Darwin)
    NUM_JOB := $(sysctl -n hw.ncpu)
else
    NUM_JOB := 1
endif

clean:
	rm -rf $(BUILD_DIR)
.PHONY: clean

# -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++"
# -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
prepare:
	mkdir -p $(BUILD_DIR) && \
	cd $(BUILD_DIR) && \
	cmake $(PROJECT_SOURCE_DIR)
.PHONY: prepare

test: prepare
	cd $(BUILD_DIR) && \
	make ciellab_test_11_exceptions_on_rtti_on ciellab_test_23_exceptions_off_rtti_off -j $(NUM_JOB) && \
	./test/ciellab_test_11_exceptions_on_rtti_on && \
	./test/ciellab_test_23_exceptions_off_rtti_off
.PHONY: test

valgrind: prepare
	cd $(BUILD_DIR) && \
	make ciellab_test_11_exceptions_on_rtti_on -j $(NUM_JOB) && \
	unbuffer valgrind --tool=memcheck --leak-check=full ./test/ciellab_test_11_exceptions_on_rtti_on | tee valgrind_output.txt && \
	grep -q "ERROR SUMMARY: 0 errors" valgrind_output.txt
.PHONY: valgrind

benchmark: prepare
	cd $(BUILD_DIR) && \
	make ciellab_benchmark -j $(NUM_JOB) && \
	./benchmark/ciellab_benchmark
.PHONY: benchmark

format:
	./format.sh run
.PHONY: format

check_format:
	./format.sh check
.PHONY: check_format
