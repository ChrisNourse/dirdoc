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

# Tiktoken integration (using OpenAI's embedding data)
TIKTOKEN_INCLUDE = -I$(SRC_DIR)
TIKTOKEN_LDFLAGS = -lstdc++
TIKTOKEN_DATA_FILE = $(SRC_DIR)/tiktoken_data.h

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

.PHONY: all clean super_clean deps help test build_temp clean_temp samples tiktoken

all: deps tiktoken $(BUILD_DIR)/dirdoc
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
		echo "✅ Cosmopolitan dependencies setup complete."; \
	else \
		echo "✅ cosmocc already exists, skipping unzip"; \
	fi

tiktoken: $(TIKTOKEN_DATA_FILE)

$(TIKTOKEN_DATA_FILE):
	@echo "⏳ Setting up OpenAI tiktoken data extraction..."
	@mkdir -p $(DEPS_DIR)
	@mkdir -p $(SRC_DIR)/tiktoken_data
	@if ! command -v python3 &> /dev/null; then \
		echo "❌ Python 3 is required for tiktoken data extraction"; \
		exit 1; \
	fi
	@if ! python3 -c "import tiktoken" &> /dev/null; then \
		echo "⏳ Installing tiktoken Python package..."; \
		pip install tiktoken; \
	fi
	@echo "⏳ Extracting OpenAI tiktoken data..."
	@python3 src/extract_tiktoken_data.py
	@echo "⏳ Generating C header from tiktoken data..."
	@python3 src/convert_to_c_header.py
	@echo "✅ OpenAI tiktoken data extraction complete"

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

$(BUILD_DIR)/dirdoc_test: $(wildcard $(SRC_DIR)/*.c) $(wildcard $(TEST_DIR)/*.c) $(CPP_OBJECTS) | $(BUILD_DIR) deps
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
	@echo "  tiktoken    - Extract OpenAI tiktoken data and generate headers"
	@echo "  clean       - Remove build artifacts"
	@echo "  super_clean - Remove build artifacts and dependencies"
	@echo "  test        - Build and run the test suite"
	@echo "  build_temp  - Build the test binary for generating temp test files (without auto-cleanup)"
	@echo "  clean_temp  - Remove all temporary test files from the 'tmp' directory"
	@echo "  samples     - Create a sample project structure to demonstrate dirdoc"
	@echo "  help        - Show this help message"
	
