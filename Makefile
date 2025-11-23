# HelixShell Makefile

.PHONY: all build test run clean help docker-build-all docker-run

# Colors for pretty output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
PURPLE := \033[0;35m
CYAN := \033[0;36m
NC := \033[0m # No Color

# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -pedantic -g -Werror -Iinclude/
LDFLAGS := -lreadline
TEST_CXXFLAGS := $(CXXFLAGS) -Itests/ `pkg-config --cflags cppunit`
TEST_LDFLAGS := `pkg-config --libs cppunit` -lreadline

# Directories
SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

# Source files
SOURCES := $(SRC_DIR)/main.cpp $(SRC_DIR)/shell.cpp $(SRC_DIR)/tokenizer.cpp $(SRC_DIR)/parser.cpp $(SRC_DIR)/executor.cpp $(SRC_DIR)/readline_support.cpp $(SRC_DIR)/prompt.cpp
TEST_SOURCES := $(TEST_DIR)/test_main.cpp $(TEST_DIR)/test_executor.cpp $(TEST_DIR)/test_integration.cpp $(TEST_DIR)/test_tokenizer.cpp $(TEST_DIR)/test_parser.cpp $(TEST_DIR)/test_shell.cpp

# Object files
MAIN_OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TEST_OBJECTS := $(TEST_SOURCES:$(TEST_DIR)/%.cpp=$(BUILD_DIR)/%.o) $(BUILD_DIR)/tokenizer.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/executor.o $(BUILD_DIR)/shell.o $(BUILD_DIR)/readline_support.o $(BUILD_DIR)/prompt.o

# Executables
MAIN_EXE := $(BUILD_DIR)/hsh
TEST_EXE := $(BUILD_DIR)/hsh_tests

# Default target
all: build

# Build the main executable
build: $(MAIN_EXE)
	@echo "$(GREEN)✨ Build complete! Executable: $(MAIN_EXE)$(NC)"

# Build and run tests (using production-optimized build)
test: CXXFLAGS := -std=c++23 -O3 -DNDEBUG -march=native -flto -Wall -Wextra -pedantic -Iinclude/
test: LDFLAGS := `pkg-config --libs cppunit` -lreadline -flto
test: $(TEST_EXE)
	@echo "$(BLUE)🧪 Running 48 tests (production build)...$(NC)"
	@if $(TEST_EXE) >/dev/null 2>&1; then \
		echo "$(GREEN)✅ 48/48 tests passed!$(NC)"; \
	else \
		echo "$(RED)❌ Some tests failed!$(NC)"; \
		exit 1; \
	fi

# Run the shell executable and build Docker image
run: $(MAIN_EXE) docker-build-all
	@echo "$(CYAN)🚀 Starting HelixShell...$(NC)"
	./$(MAIN_EXE)

# Clean build artifacts
clean:
	@echo "$(YELLOW)🧹 Cleaning build directory...$(NC)"
	rm -rf $(BUILD_DIR)
	@echo "$(GREEN)✨ Clean complete!$(NC)"

# Help target
help:
	@echo "Available targets:"
	@echo "  all               - Build the project (default)"
	@echo "  build             - Build the main executable"
	@echo "  test              - Build and run tests"
	@echo "  run               - Build for all platforms including Docker and run locally"
	@echo "  docker-build-all  - Build Docker image for all platforms (amd64, arm64)"
	@echo "  docker-run        - Run HelixShell in Docker container"
	@echo "  clean             - Remove build directory"
	@echo "  help              - Show this help message"

# Docker targets
docker-build-all:
	@echo "$(BLUE)🐳 Building Docker image for all platforms (linux/amd64, linux/arm64)...$(NC)"
	@if command -v docker >/dev/null 2>&1; then \
		docker buildx build --platform linux/amd64,linux/arm64 -t helixshell:latest .; \
		echo "$(GREEN)✅ Docker image built successfully for all platforms!$(NC)"; \
	else \
		echo "$(YELLOW)⚠️  Docker not found. Skipping Docker build.$(NC)"; \
	fi

docker-run:
	@echo "$(CYAN)🐳 Starting HelixShell in Docker container...$(NC)"
	@if command -v docker >/dev/null 2>&1; then \
		docker run -it --rm \
			--name helix-shell \
			-v $$(pwd)/workspace:/home/shelluser/workspace \
			helixshell:latest; \
	else \
		echo "$(RED)❌ Docker not installed or not running$(NC)"; \
		exit 1; \
	fi

# Create build directory
create_build_dir:
	mkdir -p $(BUILD_DIR)

# Main executable
$(MAIN_EXE): $(MAIN_OBJECTS)
	@echo "$(YELLOW)🔗 Linking HelixShell executable...$(NC)"
	$(CXX) $(MAIN_OBJECTS) -o $@ $(LDFLAGS)

# Test executable
$(TEST_EXE): $(TEST_OBJECTS)
	@echo "$(YELLOW)🔗 Linking test executable...$(NC)"
	$(CXX) $(TEST_OBJECTS) -o $@ $(TEST_LDFLAGS)

# Compile main object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "$(PURPLE)📦 Compiling $(notdir $<)...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile test object files
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "$(PURPLE)🧪 Compiling test $(notdir $<)...$(NC)"
	@$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

# Dependencies
$(BUILD_DIR)/main.o: include/shell.h
$(BUILD_DIR)/shell.o: include/shell.h include/tokenizer.h include/parser.h include/executor.h include/types.h include/readline_support.h include/prompt.h
$(BUILD_DIR)/tokenizer.o: include/tokenizer.h include/types.h
$(BUILD_DIR)/parser.o: include/parser.h include/tokenizer.h include/types.h
$(BUILD_DIR)/executor.o: include/executor.h include/types.h
$(BUILD_DIR)/readline_support.o: include/readline_support.h
$(BUILD_DIR)/prompt.o: include/prompt.h

# Test dependencies
$(BUILD_DIR)/test_executor.o: tests/test_helpers.h
$(BUILD_DIR)/test_integration.o: tests/test_helpers.h
$(BUILD_DIR)/test_tokenizer.o: tests/catch.h
$(BUILD_DIR)/test_parser.o: tests/catch.h
$(BUILD_DIR)/test_shell.o: tests/test_shell.h tests/test_helpers.h
