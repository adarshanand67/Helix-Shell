BUILD_DIR := build

.PHONY: all build test clean

all: build

build:
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug > /dev/null
	@cmake --build $(BUILD_DIR) -- -j$$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)

test: build
	@cd $(BUILD_DIR) && ./hsh_tests

clean:
	@rm -rf $(BUILD_DIR)
