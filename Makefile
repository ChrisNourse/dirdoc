# Makefile
SHELL = /bin/bash

# Directories
SRC_DIR   = src
DEPS_DIR  = deps
BUILD_DIR = build
TEST_DIR  = tests

# Compiler and Flags
CC = $(DEPS_DIR)/cosmocc/bin/cosmocc
CFLAGS = -g -O2 -Ideps/cosmocc/include $(TIKTOKEN_INCLUDE)
LDFLAGS = $(TIKTOKEN_LDFLAGS)

# Tiktoken integration
TIKTOKEN_DIR = $(DEPS_DIR)/tiktoken
TIKTOKEN_INCLUDE = -I$(TIKTOKEN_DIR) -I$(DEPS_DIR)
TIKTOKEN_LDFLAGS = -L$(TIKTOKEN_DIR)/build -ltiktoken -lstdc++

# Cosmopolitan Libc 4.0.2 URLs
COSMO_ZIP_URL = https://github.com/jart/cosmopolitan/releases/download/4.0.2/cosmocc-4.0.2.zip
COSMO_ZIP     = cosmocc-4.0.2.zip

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
# Object files: compile each source file into an object file in $(BUILD_DIR)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

# C++ compiler
CXX = $(DEPS_DIR)/cosmocc/bin/cosmoc++

# Ensure dirdoc.o is linked last (it provides get_default_output)
DIRDOC_OBJ = $(BUILD_DIR)/dirdoc.o
CPP_SOURCES = $(SRC_DIR)/tiktoken_cpp.cpp
CPP_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CPP_SOURCES))
OTHER_OBJS = $(filter-out $(DIRDOC_OBJ), $(OBJECTS)) $(CPP_OBJECTS)

.PHONY: all clean super_clean deps help test build_temp clean_temp samples build_tiktoken

all: deps $(BUILD_DIR)/dirdoc
	@echo "✅ Build completed successfully"
	@echo "📍 Binary location: $(BUILD_DIR)/dirdoc"
	@echo "🚀 Run ./$(BUILD_DIR)/dirdoc --help for usage"

deps: $(CC) build_tiktoken

$(CC): $(DEPS_DIR)/$(COSMO_ZIP)
	@echo "⏳ Checking cosmocc..."
	@if [ ! -f "$(CC)" ]; then \
		echo "⏳ Unpacking $(COSMO_ZIP)..."; \
		mkdir -p $(DEPS_DIR)/cosmocc/bin; \
		unzip -q $(DEPS_DIR)/$(COSMO_ZIP) -d $(DEPS_DIR)/cosmocc; \
		chmod +x $(CC); \
		echo "✅ Cosmopolitan dependencies setup complete."; \
	else \
		echo "✅ cosmocc already exists, skipping unzip"; \
	fi

build_tiktoken: $(DEPS_DIR)/download_tiktoken.sh
	@echo "⏳ Setting up tiktoken..."
	@mkdir -p $(DEPS_DIR)
	@if [ ! -d "$(TIKTOKEN_DIR)" ]; then \
		echo "📦 Building cpp-tiktoken..."; \
		$(DEPS_DIR)/download_tiktoken.sh; \
		echo "✅ cpp-tiktoken built successfully."; \
	else \
		echo "✅ cpp-tiktoken directory exists, skipping build"; \
	fi

$(DEPS_DIR)/download_tiktoken.sh:
	@echo "📦 Creating download script..."
	@mkdir -p $(DEPS_DIR)
	@echo '#!/bin/bash' > $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '# Script to download and build cpp-tiktoken' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'set -e' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'SCRIPT_DIR="$$(cd "$$(dirname "$${BASH_SOURCE[0]}")" && pwd)"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'TIKTOKEN_DIR="$${SCRIPT_DIR}/tiktoken"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'TIKTOKEN_REPO="https://github.com/gh-markt/cpp-tiktoken.git"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'PCRE2_DIR="$${TIKTOKEN_DIR}/pcre2"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '# Create directory if it doesn'"'"'t exist' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'mkdir -p "$${TIKTOKEN_DIR}"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '# Clone the repository if it doesn'"'"'t exist' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'if [ ! -d "$${TIKTOKEN_DIR}/.git" ]; then' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '    echo "Cloning cpp-tiktoken repository..."' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '    rm -rf "$${TIKTOKEN_DIR}"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '    git clone "$${TIKTOKEN_REPO}" "$${TIKTOKEN_DIR}"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'else' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '    echo "cpp-tiktoken repository already exists, updating..."' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '    cd "$${TIKTOKEN_DIR}" && git pull' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'fi' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '# Initialize and update submodules' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'echo "Initializing and updating submodules..."' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'cd "$${TIKTOKEN_DIR}" && git submodule update --init --recursive' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '# Create build directory' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'mkdir -p "$${TIKTOKEN_DIR}/build"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '# Build and install' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'cd "$${TIKTOKEN_DIR}/build"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'echo "Building cpp-tiktoken..."' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$${TIKTOKEN_DIR}/install"' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'make -j$$(nproc)' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'make install' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo '' >> $(DEPS_DIR)/download_tiktoken.sh
	@echo 'echo "cpp-tiktoken has been built and installed to $${TIKTOKEN_DIR}/install"' >> $(DEPS_DIR)/download_tiktoken.sh
	@chmod +x $(DEPS_DIR)/download_tiktoken.sh

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

# Compile each source file into an object file.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

# Link all object files together, ensuring dirdoc.o is last.
$(BUILD_DIR)/dirdoc: $(OTHER_OBJS) $(DIRDOC_OBJ)
	@echo "⏳ Linking dirdoc..."
	$(CC) $(LDFLAGS) -o $@ $(OTHER_OBJS) $(DIRDOC_OBJ)
	@echo "✅ Build complete"

$(BUILD_DIR)/dirdoc_test: $(wildcard $(SRC_DIR)/*.c) $(wildcard $(TEST_DIR)/*.c) | $(BUILD_DIR) deps
	@echo "⏳ Building tests..."
	$(CC) $(CFLAGS) -DUNIT_TEST -I. -I$(SRC_DIR) -I$(TEST_DIR) -Ideps/cosmocc/include -o $@ $^ $(LDFLAGS)
	@echo "✅ Test build complete"

$(BUILD_DIR)/temp_test: $(wildcard $(SRC_DIR)/*.c) $(wildcard $(TEST_DIR)/*.c) | $(BUILD_DIR) deps
	@echo "⏳ Building temp test binary..."
	$(CC) $(CFLAGS) -DUNIT_TEST -DINSPECT_TEMP -I. -I$(SRC_DIR) -I$(TEST_DIR) -Ideps/cosmocc/include -o $@ $^ $(LDFLAGS)
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
	@echo "  build_tiktoken - Build just the tiktoken library"
	@echo "  clean       - Remove build artifacts"
	@echo "  super_clean - Remove build artifacts and dependencies"
	@echo "  test        - Build and run the test suite"
	@echo "  build_temp  - Build the test binary for generating temp test files (without auto-cleanup)"
	@echo "  clean_temp  - Remove all temporary test files from the 'tmp' directory"
	@echo "  samples     - Create a sample project structure to demonstrate dirdoc"
	@echo "  help        - Show this help message"
	
