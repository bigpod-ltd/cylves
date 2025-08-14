# Cylves Development Makefile

# Configuration
BUILD_DIR ?= build
BUILD_TYPE ?= Debug
CMAKE_OPTS ?=
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Directories
SRC_DIR = reference/sylves-c
DOCS_DIR = $(SRC_DIR)/docs

# Colors for output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[0;33m
NC = \033[0m # No Color

.PHONY: all build clean test coverage docs format check help

# Default target
all: build

## help: Show this help message
help:
	@echo "Cylves Development Makefile"
	@echo ""
	@echo "Usage: make [target] [VAR=value]"
	@echo ""
	@echo "Targets:"
	@grep -E '^## ' $(MAKEFILE_LIST) | sed 's/## /  /'
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_TYPE    Build type (Debug/Release) [$(BUILD_TYPE)]"
	@echo "  BUILD_DIR     Build directory [$(BUILD_DIR)]"
	@echo "  JOBS          Parallel jobs [$(JOBS)]"

## build: Build the library
build: $(BUILD_DIR)/Makefile
	@echo "$(GREEN)Building Cylves ($(BUILD_TYPE))...$(NC)"
	@cmake --build $(BUILD_DIR) -j$(JOBS)

## configure: Configure the build with CMake
configure:
	@echo "$(GREEN)Configuring build...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DBUILD_TESTS=ON \
		-DBUILD_EXAMPLES=ON \
		$(CMAKE_OPTS) \
		../$(SRC_DIR)

$(BUILD_DIR)/Makefile:
	@$(MAKE) configure

## clean: Clean build artifacts
clean:
	@echo "$(YELLOW)Cleaning build artifacts...$(NC)"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(DOCS_DIR)/html
	@rm -rf $(DOCS_DIR)/latex
	@find . -name "*.o" -delete
	@find . -name "*.a" -delete
	@find . -name "*.so" -delete
	@find . -name "*.dylib" -delete

## test: Run all tests
test: build
	@echo "$(GREEN)Running tests...$(NC)"
	@cd $(BUILD_DIR) && ctest --output-on-failure -j$(JOBS)

## test-verbose: Run tests with verbose output
test-verbose: build
	@echo "$(GREEN)Running tests (verbose)...$(NC)"
	@cd $(BUILD_DIR) && ctest -V

## test-memcheck: Run tests with memory checking (Linux only)
test-memcheck: build
	@echo "$(GREEN)Running tests with Valgrind...$(NC)"
	@cd $(BUILD_DIR) && ctest -T memcheck --output-on-failure

## coverage: Generate code coverage report
coverage:
	@echo "$(GREEN)Generating coverage report...$(NC)"
	@$(MAKE) clean
	@$(MAKE) configure BUILD_TYPE=Debug CMAKE_OPTS="-DCMAKE_C_FLAGS='--coverage -fprofile-arcs -ftest-coverage'"
	@$(MAKE) build
	@$(MAKE) test
	@gcovr --root $(SRC_DIR)/src \
		--exclude ".*test.*" \
		--exclude ".*example.*" \
		--print-summary \
		--html-details coverage.html
	@echo "$(GREEN)Coverage report generated: coverage.html$(NC)"

## docs: Generate documentation
docs:
	@echo "$(GREEN)Generating documentation...$(NC)"
	@cd $(SRC_DIR) && doxygen Doxyfile
	@echo "$(GREEN)Documentation generated: $(DOCS_DIR)/html/index.html$(NC)"

## format: Format all source code
format:
	@echo "$(GREEN)Formatting code...$(NC)"
	@find $(SRC_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i

## check-format: Check code formatting without modifying
check-format:
	@echo "$(GREEN)Checking code format...$(NC)"
	@find $(SRC_DIR) -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror

## lint: Run static analysis
lint:
	@echo "$(GREEN)Running static analysis...$(NC)"
	@cppcheck --enable=all --error-exitcode=1 \
		--suppress=missingIncludeSystem \
		--inline-suppr --quiet \
		-I $(SRC_DIR)/src/include \
		$(SRC_DIR)/src/

## install: Install the library
install: build
	@echo "$(GREEN)Installing Cylves...$(NC)"
	@cmake --install $(BUILD_DIR)

## uninstall: Uninstall the library
uninstall:
	@echo "$(YELLOW)Uninstalling Cylves...$(NC)"
	@if [ -f $(BUILD_DIR)/install_manifest.txt ]; then \
		xargs rm -f < $(BUILD_DIR)/install_manifest.txt; \
		echo "$(GREEN)Uninstall complete$(NC)"; \
	else \
		echo "$(RED)No installation found$(NC)"; \
	fi

## debug: Build and run with debugger
debug: build
	@echo "$(GREEN)Starting debugger...$(NC)"
	@cd $(BUILD_DIR) && gdb ./tests/test_sylves

## benchmark: Run performance benchmarks
benchmark: build
	@echo "$(GREEN)Running benchmarks...$(NC)"
	@cd $(BUILD_DIR) && ./benchmarks/benchmark_grids

## dev-setup: Set up development environment
dev-setup:
	@echo "$(GREEN)Setting up development environment...$(NC)"
	@pip install pre-commit gcovr
	@pre-commit install
	@echo "$(GREEN)Development environment ready!$(NC)"

## ci-local: Run CI pipeline locally
ci-local: clean
	@echo "$(GREEN)Running local CI pipeline...$(NC)"
	@$(MAKE) check-format
	@$(MAKE) lint
	@$(MAKE) build BUILD_TYPE=Debug
	@$(MAKE) test
	@$(MAKE) build BUILD_TYPE=Release
	@$(MAKE) test
	@$(MAKE) coverage
	@$(MAKE) docs
	@echo "$(GREEN)Local CI pipeline complete!$(NC)"

## status: Show project status
status:
	@echo "$(GREEN)Project Status:$(NC)"
	@echo "  Build directory: $(BUILD_DIR)"
	@echo "  Build type: $(BUILD_TYPE)"
	@echo "  Source files:"
	@find $(SRC_DIR)/src -name "*.c" | wc -l | xargs echo "    C files:"
	@find $(SRC_DIR)/src -name "*.h" | wc -l | xargs echo "    Header files:"
	@echo "  Test files:"
	@find $(SRC_DIR)/tests -name "*.c" 2>/dev/null | wc -l | xargs echo "    Test files:"
	@echo "  Documentation:"
	@if [ -d $(DOCS_DIR)/html ]; then echo "    Generated"; else echo "    Not generated"; fi

## update-gap: Update gap analysis with current progress
update-gap:
	@echo "$(YELLOW)Note: Manual update of GAP_ANALYSIS.md required$(NC)"
	@echo "Current implementation files:"
	@find $(SRC_DIR)/src -name "*.c" -exec basename {} \; | sort

# Quick targets for common operations
b: build
t: test
c: clean
d: docs
f: format

.DEFAULT_GOAL := help
