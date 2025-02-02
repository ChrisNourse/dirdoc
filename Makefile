SHELL = /bin/bash

# Directories
SRC_DIR   = src
DEPS_DIR  = deps
BUILD_DIR = build
TEST_DIR  = tests

# Compiler and Flags
CC = $(DEPS_DIR)/cosmocc/bin/cosmocc
CFLAGS = -g -O2
LDFLAGS = 

# Cosmopolitan Libc 4.0.2 URLs
COSMO_ZIP_URL = https://github.com/jart/cosmopolitan/releases/download/4.0.2/cosmocc-4.0.2.zip
COSMO_ZIP     = cosmocc-4.0.2.zip

# Dynamically collect all .c files from src
SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Test source(s)
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)

.PHONY: all clean super_clean deps help test build_temp clean_temp

all: deps $(BUILD_DIR)/dirdoc
	@echo "✅ Build completed successfully"
	@echo "📍 Binary location: $(BUILD_DIR)/dirdoc"
	@echo "🚀 Run ./$(BUILD_DIR)/dirdoc --help for usage"

deps: $(CC)

$(CC): $(DEPS_DIR)/$(COSMO_ZIP)
	@echo "⏳ Checking cosmocc..."
	@if [ ! -f "$(CC)" ]; then \
		echo "⏳ Unpacking $(COSMO_ZIP)..."; \
		mkdir -p $(DEPS_DIR)/cosmocc/bin; \
		unzip -q $(DEPS_DIR)/$(COSMO_ZIP) -d $(DEPS_DIR)/cosmocc; \
		chmod +x $(CC); \
		echo "✅ Dependencies setup complete."; \
	else \
		echo "✅ cosmocc already exists, skipping unzip"; \
	fi

$(DEPS_DIR)/$(COSMO_ZIP):
	@mkdir -p $(DEPS_DIR)
	@if [ ! -f "$(DEPS_DIR)/$(COSMO_ZIP)" ]; then \
		echo "📦 Fetching Cosmopolitan 4.0.2 dependencies..."; \
		curl -L $(COSMO_ZIP_URL) -o $(DEPS_DIR)/$(COSMO_ZIP); \
		echo "✅ Downloaded $(COSMO_ZIP)"; \
	else \
		echo "✅ $(COSMO_ZIP) already exists, skipping download"; \
	fi

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/dirdoc: $(SOURCES) | $(BUILD_DIR) deps
	@echo "⏳ Building dirdoc..."
	$(CC) $(CFLAGS) -I$(DEPS_DIR)/cosmocc/include -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete"

$(BUILD_DIR)/dirdoc_test: $(TEST_SOURCES) $(SOURCES) | $(BUILD_DIR) deps
	@echo "⏳ Building tests..."
	$(CC) $(CFLAGS) -DUNIT_TEST -I. -I$(SRC_DIR) -I$(TEST_DIR) -I$(DEPS_DIR)/cosmocc/include -o $@ $^ $(LDFLAGS)
	@echo "✅ Test build complete"

# Updated target to build a test binary that does not clean up temp files (for manual inspection)
$(BUILD_DIR)/temp_test: $(TEST_SOURCES) $(SOURCES) | $(BUILD_DIR) deps
	@echo "⏳ Building temp test binary..."
	$(CC) $(CFLAGS) -DUNIT_TEST -DINSPECT_TEMP -I. -I$(SRC_DIR) -I$(TEST_DIR) -I$(DEPS_DIR)/cosmocc/include -o $@ $^ $(LDFLAGS)
	@echo "✅ Temp test build complete"

build_temp: deps $(BUILD_DIR)/temp_test
	@echo "✅ Temp test binary built. Run './$(BUILD_DIR)/temp_test' to generate temporary test files for inspection."

clean_temp:
	@echo "⏳ Removing temporary test files..."
	rm -rf tmp
	@echo "✅ Temp test files removed"

test: deps $(BUILD_DIR)/dirdoc_test
	@echo "🚀 Running tests..."
	./$(BUILD_DIR)/dirdoc_test

clean:
	@echo "⏳ Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "✅ Clean complete"

super_clean:
	@echo "⏳ Cleaning build artifacts and dependencies..."
	rm -rf $(BUILD_DIR) $(DEPS_DIR)
	@echo "✅ Clean complete"

help:
	@echo "Available targets:"
	@echo "  all         - Build the dirdoc application"
	@echo "  deps        - Download and set up dependencies"
	@echo "  clean       - Remove build artifacts"
	@echo "  super_clean - Remove build artifacts and dependencies"
	@echo "  test        - Build and run the test suite"
	@echo "  build_temp  - Build the test binary for generating temp test files (without auto-cleanup)"
	@echo "  clean_temp  - Remove all temporary test files from the 'tmp' directory"
	@echo "  help        - Show this help message"
