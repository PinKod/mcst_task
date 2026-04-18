BUILD_DIR              := build
BUILD_RELEASE_DIR      := build_release
LINTERS_DIR            := linters
CLANG_FORMAT_FILE      := $(LINTERS_DIR)/.clang-format
IGNORE_DIRS            := -path ./$(BUILD_DIR) -o -path ./$(BUILD_DEBUG_DIR) \
                       -o -path ./$(BUILD_RELEASE_DIR) -o -path ./$(BUILD_ASAN_DIR) \
                       -o -path ./$(BUILD_TSAN_DIR) -o -path ./$(BUILD_UBSAN_DIR) \
                       -o -path ./$(BUILD_MSAN_DIR) -o -path ./$(BUILD_LSAN_DIR) \
                       -o -path ./$(BUILD_COVERAGE_DIR) -o -path ./$(BUILD_PERF_DIR) \
                       -o -path ./$(BUILD_PERF_PGO_DIR) -o -path ./$(BUILD_PERF_PGO_USE_DIR) \
                       -o -path ./$(REPORTS_DIR)

MKDIR                  := mkdir -p
RM                     := rm -rf
CMAKE                  := cmake
CLANG_FORMAT           := clang-format

PROJECT_ROOT           := .
TEST_DATA_DIR          := $(PROJECT_ROOT)/test_data
RESULTS_DIR            := $(PROJECT_ROOT)/results

.PHONY: all clean config build

all : format config build

config:
	@$(MKDIR) $(BUILD_DIR)
	$(CMAKE) -B $(BUILD_DIR) -S $(PROJECT_ROOT) -DCMAKE_BUILD_TYPE=Release

build:
	@$(CMAKE) --build $(BUILD_DIR) -- -j$$(nproc)

release: config
	$(CMAKE) --build $(BUILD_DIR) --config Release -- -j$$(nproc)

debug:
	$(MKDIR) $(BUILD_DIR)
	$(CMAKE) -B $(BUILD_DIR) -S $(PROJECT_ROOT) -DCMAKE_BUILD_TYPE=Debug
	$(CMAKE) --build $(BUILD_DIR) --config Debug -- -j$$(nproc)

clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(TEST_DATA_DIR)
	$(RM) $(RESULTS_DIR)

format:
	@if [ -f "$(CLANG_FORMAT_FILE)" ]; then \
		find . \( $(IGNORE_DIRS) \) -prune -o \
			\( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) -print 2>/dev/null | \
			grep -v "^\./\(build\|_deps\)" | \
			xargs -r $(CLANG_FORMAT) -i -style=file:$(CLANG_FORMAT_FILE); \
		echo "$(GREEN)✓ Formatting complete (using $(CLANG_FORMAT_FILE))$(RESET)"; \
	else \
		find . \( $(IGNORE_DIRS) \) -prune -o \
			\( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) -print 2>/dev/null | \
			grep -v "^\./\(build\|_deps\)" | \
			xargs -r $(CLANG_FORMAT) -i -style="{BasedOnStyle: Google, IndentWidth: 4}"; \
		echo "$(GREEN)✓ Formatting complete (using Google style)$(RESET)"; \
	fi
