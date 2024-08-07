PROJECT_SOURCE_DIR ?= $(abspath ./)
BUILD_DIR ?= $(PROJECT_SOURCE_DIR)/build
NUM_JOB ?= 32

clean:
	rm -rf $(BUILD_DIR)
.PHONY: clean

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
