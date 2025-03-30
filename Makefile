# Makefile
SHELL = /bin/bash

# Directories
SRC_DIR   = src
TOOLS_DIR = tools
DEPS_DIR  = deps
BUILD_DIR = build
TEST_DIR  = tests

# Compiler and Flags
CC = $(DEPS_DIR)/cosmocc/bin/cosmocc
CXX = $(DEPS_DIR)/cosmocc/bin/cosmoc++

# Common compiler flags for both C and C++
COMMON_FLAGS = -g -O2 -Ideps/cosmocc/include

# C-specific flags (no TIKTOKEN_INCLUDE as it's C++ specific)
CFLAGS = $(COMMON_FLAGS)

# C++-specific flags
CXXFLAGS = $(COMMON_FLAGS) $(TIKTOKEN_INCLUDE)

LDFLAGS = $(TIKTOKEN_LDFLAGS)

# Tiktoken integration (downloading data file directly)
TIKTOKEN_ENCODER_NAME = cl100k_base# Or change to gpt2, r50k_base, p50k_base, p50k_edit, etc.
TIKTOKEN_BASE_URL = https://openaipublic.blob.core.windows.net/encodings
TIKTOKEN_DATA_URL = $(TIKTOKEN_BASE_URL)/$(TIKTOKEN_ENCODER_NAME).tiktoken
TIKTOKEN_DOWNLOADED_FILE = $(BUILD_DIR)/$(TIKTOKEN_ENCODER_NAME).tiktoken # Store downloaded file in build dir
TIKTOKEN_GEN_TOOL_SRC = $(TOOLS_DIR)/generate_tiktoken_data.cpp
TIKTOKEN_GEN_TOOL_OBJ = $(BUILD_DIR)/generate_tiktoken_data.o
TIKTOKEN_GEN_TOOL_EXE = $(BUILD_DIR)/generate_tiktoken_data
TIKTOKEN_GENERATED_HEADER = $(BUILD_DIR)/tiktoken_data.h
TIKTOKEN_INCLUDE = -I$(BUILD_DIR) -I$(SRC_DIR) # Include build dir for generated header
TIKTOKEN_LDFLAGS = -lstdc++

# Cosmopolitan Libc 4.0.2 URLs
COSMO_ZIP_URL = https://github.com/jart/cosmopolitan/releases/download/4.0.2/cosmocc-4.0.2.zip
COSMO_ZIP     = cosmocc-4.0.2.zip

# Source files (exclude the old data header if it was a .c file, which it wasn't)
SOURCES = $(wildcard $(SRC_DIR)/*.c)
# Object files: compile each source file into an object file in $(BUILD_DIR)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

# Ensure dirdoc.o is linked last (it provides get_default_output)
DIRDOC_OBJ = $(BUILD_DIR)/dirdoc.o
# Main C++ sources
MAIN_CPP_SOURCES = $(SRC_DIR)/tiktoken_cpp.cpp
MAIN_CPP_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(MAIN_CPP_SOURCES))
# All C++ objects (main + tools)
ALL_CPP_OBJECTS = $(MAIN_CPP_OBJECTS) $(TIKTOKEN_GEN_TOOL_OBJ)
# Objects for the final dirdoc executable
DIRDOC_LINK_OBJS = $(filter-out $(DIRDOC_OBJ), $(OBJECTS)) $(MAIN_CPP_OBJECTS)

# Test objects need to include dirdoc.o explicitly to resolve get_default_output()
TEST_LINK_OBJS = $(OBJECTS) $(MAIN_CPP_OBJECTS) $(TEST_OBJECTS)

# Test source files and object files
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/test_%.o, $(TEST_SOURCES))

# Test-specific flags
TEST_CFLAGS = -DUNIT_TEST -I. -I$(SRC_DIR) -I$(TEST_DIR) -Ideps/cosmocc/include

# Specific objects needed for test_tiktoken
TEST_TIKTOKEN_SRCS = test_tiktoken.c
TEST_TIKTOKEN_OBJ = $(patsubst %.c, $(BUILD_DIR)/test_%.o, $(TEST_TIKTOKEN_SRCS))
TIKTOKEN_TEST_DEPS = $(BUILD_DIR)/tiktoken.o $(BUILD_DIR)/stats.o $(BUILD_DIR)/tiktoken_cpp.o


.PHONY: all clean super_clean deps deps_cosmo help test build_temp clean_temp test_tiktoken tools

# Main build target depends on the final binary
all: $(BUILD_DIR)/dirdoc
	@echo "✅ Build completed successfully"
	@echo "📍 Binary location: $(BUILD_DIR)/dirdoc"
	@echo "🚀 Run ./$(BUILD_DIR)/dirdoc --help for usage"

# Combined dependencies target (only Cosmo now)
deps: deps_cosmo


# Target for building tools
tools: $(TIKTOKEN_GEN_TOOL_EXE)

# Cosmopolitan dependency target
deps_cosmo: $(CC)

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

# Target to download the tiktoken data file
$(TIKTOKEN_DOWNLOADED_FILE): | $(BUILD_DIR)
	@echo "⏳ Checking for tiktoken data file $(TIKTOKEN_DOWNLOADED_FILE)..."
	@if [ ! -f "$@" ]; then \
		echo "📦 Downloading tiktoken data from $(TIKTOKEN_DATA_URL)..."; \
		curl -L -o $@ "$(TIKTOKEN_DATA_URL)"; \
		echo "✅ Tiktoken data file downloaded."; \
	else \
		echo "✅ Tiktoken data file already exists, skipping download"; \
	fi

# Target to generate the tiktoken C header from the downloaded repo data
# Requires the compiled C++ generator tool and the downloaded data file.
$(TIKTOKEN_GENERATED_HEADER): $(TIKTOKEN_GEN_TOOL_EXE) $(TIKTOKEN_DOWNLOADED_FILE) | $(BUILD_DIR)
	@echo "⏳ Generating tiktoken C header $(TIKTOKEN_GENERATED_HEADER)..."
	@if [ ! -f "$@" ] || [ "$(TIKTOKEN_DOWNLOADED_FILE)" -nt "$@" ]; then \
		echo "⚙️ Running $(TIKTOKEN_GEN_TOOL_EXE)..."; \
		./$(TIKTOKEN_GEN_TOOL_EXE) $(TIKTOKEN_DOWNLOADED_FILE) $@ $(TIKTOKEN_ENCODER_NAME); \
		echo "✅ Tiktoken C header generation complete."; \
	else \
		echo "✅ Tiktoken C header already exists and is up to date, skipping generation"; \
	fi

# Rule to build the generator tool executable
$(TIKTOKEN_GEN_TOOL_EXE): $(TIKTOKEN_GEN_TOOL_OBJ) | $(BUILD_DIR) deps_cosmo
	@echo "⏳ Linking generator tool $(TIKTOKEN_GEN_TOOL_EXE)..."
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "✅ Generator tool linked."

# Rule to compile the generator tool object file
$(TIKTOKEN_GEN_TOOL_OBJ): $(TIKTOKEN_GEN_TOOL_SRC) | $(BUILD_DIR) deps_cosmo
	@echo "⏳ Compiling generator tool $<..."
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@ # Include src for base64.h
	@echo "✅ Generator tool compiled."


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

# Compile main C++ files, ensuring tiktoken_cpp.o depends on the generated header
$(BUILD_DIR)/tiktoken_cpp.o: $(SRC_DIR)/tiktoken_cpp.cpp $(TIKTOKEN_GENERATED_HEADER) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/tiktoken_cpp.cpp -o $@

# Compile test source files into object files
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c $< -o $@

# Link all object files together for the main executable, ensuring dirdoc.o is last.
# Make sure the generated header exists before linking.
$(BUILD_DIR)/dirdoc: $(DIRDOC_LINK_OBJS) $(DIRDOC_OBJ) | deps
	@echo "⏳ Linking dirdoc..."
	$(CXX) $(LDFLAGS) -o $@ $(DIRDOC_LINK_OBJS) $(DIRDOC_OBJ)
	@echo "✅ Build complete"

# Test-specific version of dirdoc.o that gets compiled with the UNIT_TEST define
$(BUILD_DIR)/dirdoc_test.o: $(SRC_DIR)/dirdoc.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c $< -o $@

# Link test objects and application objects for the main test executable - using test-specific dirdoc_test.o
$(BUILD_DIR)/dirdoc_test: $(filter-out $(BUILD_DIR)/dirdoc.o, $(OBJECTS)) $(BUILD_DIR)/dirdoc_test.o $(MAIN_CPP_OBJECTS) $(TEST_OBJECTS) $(TIKTOKEN_GENERATED_HEADER) | $(BUILD_DIR) deps
	@echo "⏳ Linking test executable..."
	$(CXX) $(LDFLAGS) -o $@ $(filter-out $(TIKTOKEN_GENERATED_HEADER), $^)
	@echo "✅ Test link complete"

# Link test objects and application objects for the temp test executable
$(BUILD_DIR)/temp_test: $(filter-out $(DIRDOC_OBJ), $(OBJECTS)) $(MAIN_CPP_OBJECTS) $(TEST_OBJECTS) $(TIKTOKEN_GENERATED_HEADER) | $(BUILD_DIR) deps
	@echo "⏳ Linking temp test executable..."
	$(CXX) $(LDFLAGS) -DINSPECT_TEMP -o $@ $(filter-out $(TIKTOKEN_GENERATED_HEADER), $^)
	@echo "✅ Temp test link complete"

# Link tiktoken test objects with required application objects
$(BUILD_DIR)/test_tiktoken: $(TEST_TIKTOKEN_OBJ) $(TIKTOKEN_TEST_DEPS) $(TIKTOKEN_GENERATED_HEADER) | $(BUILD_DIR) deps
	@echo "⏳ Linking tiktoken test executable..."
	$(CXX) $(LDFLAGS) -o $@ $(filter-out $(TIKTOKEN_GENERATED_HEADER), $^)
	@echo "✅ Tiktoken test link complete"

build_temp: deps $(BUILD_DIR)/temp_test
	@echo "✅ Temp test binary built. Run './$(BUILD_DIR)/temp_test' to generate temporary test files for inspection."

clean_temp:
	@echo "⏳ Removing temporary test files..."
	rm -rf tmp
	@echo "✅ Temp test files removed"

# Update test target dependencies
test: $(BUILD_DIR)/dirdoc_test $(BUILD_DIR)/test_tiktoken
	@echo "🚀 Running main tests..."
	./$(BUILD_DIR)/dirdoc_test
	@echo "🚀 Running tiktoken tests..."
	./$(BUILD_DIR)/test_tiktoken

# Update test_tiktoken target dependencies (depends on the built test binary)
test_tiktoken: $(BUILD_DIR)/test_tiktoken
	@echo "🚀 Running tiktoken tests..."
	./$(BUILD_DIR)/test_tiktoken

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
	@echo "  all         - Build the dirdoc application (downloads deps, generates header)"
	@echo "  deps        - Download and set up dependencies (Cosmopolitan)"
	@echo "  deps_cosmo  - Download and set up Cosmopolitan dependency"
	@echo "  tools       - Build helper tools (e.g., tiktoken data generator)"
	@echo "  $(TIKTOKEN_DOWNLOADED_FILE) - Download the tiktoken data file"
	@echo "  $(TIKTOKEN_GENERATED_HEADER) - Generate the tiktoken C header (requires compiled C++ tool and downloaded data)"
	@echo "  clean       - Remove build artifacts (including tools and downloaded data)"
	@echo "  super_clean - Remove build artifacts and dependencies"
	@echo "  test        - Build and run the test suite"
	@echo "  build_temp  - Build the test binary for generating temp test files (without auto-cleanup)"
	@echo "  clean_temp  - Remove all temporary test files from the 'tmp' directory"
	@echo "  help        - Show this help message"
	
