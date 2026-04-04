BUILD_DIR              := build
BUILD_DEBUG_DIR        := build_debug
BUILD_RELEASE_DIR      := build_release
BUILD_PERF_DIR         := build_perf

MKDIR                  := mkdir -p
RM                     := rm -rf
CMAKE                  := cmake

all :
	@$(MKDIR) $(BUILD_DIR)
	$(CMAKE) -B $(BUILD_DIR) -S .
	@$(CMAKE) --build $(BUILD_DIR) -- -j$(NPROC)