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
TIKTOKEN_DATA_DIR = $(DEPS_DIR)/tiktoken
TIKTOKEN_DOWNLOADED_FILE = $(TIKTOKEN_DATA_DIR)/$(TIKTOKEN_ENCODER_NAME).tiktoken
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


.PHONY: all clean super_clean deps test help

# Main build target depends on the final binary
all: $(BUILD_DIR)/dirdoc
	@echo "✅ Build completed successfully"
	@echo "📍 Binary location: $(BUILD_DIR)/dirdoc"
	@echo "🚀 Run ./$(BUILD_DIR)/dirdoc --help for usage"

# Combined dependencies target (only Cosmo now)
deps: ensure_dirs deps_cosmo download_tiktoken

# Ensure all required directories exist
ensure_dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(DEPS_DIR)
	@mkdir -p $(TIKTOKEN_DATA_DIR)
	@mkdir -p $(DEPS_DIR)/cosmocc/bin

# Cosmopolitan dependency target
deps_cosmo: ensure_dirs $(CC)

$(CC): $(DEPS_DIR)/$(COSMO_ZIP)
	@echo "⏳ Checking cosmocc..."
	@if [ ! -f "$(CC)" ]; then \
		echo "⏳ Unpacking $(COSMO_ZIP)..."; \
		unzip -q $(DEPS_DIR)/$(COSMO_ZIP) -d $(DEPS_DIR)/cosmocc; \
		chmod +x $(CC); \
		echo "✅ Cosmopolitan dependencies setup complete."; \
	else \
		echo "✅ cosmocc already exists, skipping unzip"; \
	fi

# Target for tiktoken data to help debugging
download_tiktoken: ensure_dirs $(TIKTOKEN_DOWNLOADED_FILE)

# Target to download the tiktoken data file - used touch to create empty file if download fails
.INTERMEDIATE: $(TIKTOKEN_DOWNLOADED_FILE).tmp
$(TIKTOKEN_DOWNLOADED_FILE).tmp:
	@touch $@

# The actual tiktoken data file target with verbose debug info
$(TIKTOKEN_DOWNLOADED_FILE): ensure_dirs
	@echo "⏳ Checking for tiktoken data file at: $(TIKTOKEN_DOWNLOADED_FILE)"
	@echo "  Data dir exists: $$([ -d "$(TIKTOKEN_DATA_DIR)" ] && echo "YES" || echo "NO")"
	@echo "  File exists: $$([ -f "$(TIKTOKEN_DOWNLOADED_FILE)" ] && echo "YES" || echo "NO")"
	@if [ ! -f "$(TIKTOKEN_DOWNLOADED_FILE)" ]; then \
		echo "📦 Downloading tiktoken data from $(TIKTOKEN_DATA_URL)..."; \
		curl -L -s -o "$(TIKTOKEN_DOWNLOADED_FILE)" "$(TIKTOKEN_DATA_URL)" || touch "$(TIKTOKEN_DOWNLOADED_FILE).failed"; \
		if [ -f "$(TIKTOKEN_DOWNLOADED_FILE).failed" ]; then \
			echo "❌ Download failed! The URL might have changed."; \
			echo "   Please check if the Tiktoken data is still available at:"; \
			echo "   $(TIKTOKEN_DATA_URL)"; \
			echo "   You may need to update the URL in the Makefile if the resource has moved."; \
			rm -f "$(TIKTOKEN_DOWNLOADED_FILE).failed"; \
			exit 1; \
		fi; \
		echo "✅ Tiktoken data file downloaded to $(TIKTOKEN_DOWNLOADED_FILE)"; \
		ls -la "$(TIKTOKEN_DOWNLOADED_FILE)"; \
	else \
		echo "✅ Tiktoken data file already exists at $(TIKTOKEN_DOWNLOADED_FILE), size: $$(du -h "$(TIKTOKEN_DOWNLOADED_FILE)" | cut -f1)"; \
		ls -la "$(TIKTOKEN_DOWNLOADED_FILE)"; \
	fi
	@touch $(TIKTOKEN_DOWNLOADED_FILE)

# Target to generate the tiktoken C header from the downloaded repo data
# Requires the compiled C++ generator tool and the downloaded data file.
$(TIKTOKEN_GENERATED_HEADER): $(TIKTOKEN_GEN_TOOL_EXE) $(TIKTOKEN_DOWNLOADED_FILE)
	@echo "⏳ Generating tiktoken C header $(TIKTOKEN_GENERATED_HEADER)..."
	@if [ ! -f "$(TIKTOKEN_GENERATED_HEADER)" ] || [ "$(TIKTOKEN_DOWNLOADED_FILE)" -nt "$(TIKTOKEN_GENERATED_HEADER)" ]; then \
		echo "⚙️ Running $(TIKTOKEN_GEN_TOOL_EXE)..."; \
		./$(TIKTOKEN_GEN_TOOL_EXE) "$(TIKTOKEN_DOWNLOADED_FILE)" "$(TIKTOKEN_GENERATED_HEADER)" $(TIKTOKEN_ENCODER_NAME); \
		echo "✅ Tiktoken C header generation complete."; \
	else \
		echo "✅ Tiktoken C header already exists and is up to date, skipping generation"; \
	fi

# Rule to build the generator tool executable
$(TIKTOKEN_GEN_TOOL_EXE): $(TIKTOKEN_GEN_TOOL_OBJ) | deps_cosmo
	@echo "⏳ Linking generator tool $(TIKTOKEN_GEN_TOOL_EXE)..."
	$(CXX) $(LDFLAGS) -o $@ $(TIKTOKEN_GEN_TOOL_OBJ)
	@echo "✅ Generator tool linked."

# Rule to compile the generator tool object file
$(TIKTOKEN_GEN_TOOL_OBJ): $(TIKTOKEN_GEN_TOOL_SRC) | deps_cosmo
	@echo "⏳ Compiling generator tool $<..."
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@ # Include src for base64.h
	@echo "✅ Generator tool compiled."


$(DEPS_DIR)/$(COSMO_ZIP): ensure_dirs
	@if [ ! -f "$(DEPS_DIR)/$(COSMO_ZIP)" ]; then \
		echo "📦 Fetching Cosmopolitan 4.0.2 dependencies..."; \
		curl -L -o $(DEPS_DIR)/$(COSMO_ZIP) $(COSMO_ZIP_URL) || touch "$(DEPS_DIR)/$(COSMO_ZIP).failed"; \
		if [ -f "$(DEPS_DIR)/$(COSMO_ZIP).failed" ]; then \
			echo "❌ Download failed! The URL for Cosmopolitan might have changed."; \
			echo "   Please check if Cosmopolitan 4.0.2 is still available at:"; \
			echo "   $(COSMO_ZIP_URL)"; \
			echo "   You may need to update the version or URL in the Makefile if a new version is available."; \
			echo "   Current version: 4.0.2"; \
			rm -f "$(DEPS_DIR)/$(COSMO_ZIP).failed"; \
			exit 1; \
		fi; \
		echo "✅ Downloaded $(COSMO_ZIP)"; \
	else \
		echo "✅ $(COSMO_ZIP) already exists, skipping download"; \
	fi

# Compile each source file into an object file.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c deps
	$(CC) $(CFLAGS) -c $< -o $@

# Compile main C++ files, ensuring tiktoken_cpp.o depends on the generated header
$(BUILD_DIR)/tiktoken_cpp.o: $(SRC_DIR)/tiktoken_cpp.cpp $(TIKTOKEN_GENERATED_HEADER) deps
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/tiktoken_cpp.cpp -o $@

# Compile test source files into object files
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.c deps
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c $< -o $@

# Link all object files together for the main executable, ensuring dirdoc.o is last.
# Make sure the generated header exists before linking.
$(BUILD_DIR)/dirdoc: $(DIRDOC_LINK_OBJS) $(DIRDOC_OBJ) deps
	@echo "⏳ Linking dirdoc..."
	$(CXX) $(LDFLAGS) -o $@ $(DIRDOC_LINK_OBJS) $(DIRDOC_OBJ)
	@echo "✅ Build complete"

# Test-specific version of dirdoc.o that gets compiled with the UNIT_TEST define
$(BUILD_DIR)/dirdoc_test.o: $(SRC_DIR)/dirdoc.c deps
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -c $< -o $@

# Link test objects and application objects for the main test executable - using test-specific dirdoc_test.o
$(BUILD_DIR)/dirdoc_test: $(filter-out $(BUILD_DIR)/dirdoc.o, $(OBJECTS)) $(BUILD_DIR)/dirdoc_test.o $(MAIN_CPP_OBJECTS) $(TEST_OBJECTS) $(TIKTOKEN_GENERATED_HEADER) | deps
	@echo "⏳ Linking test executable..."
	$(CXX) $(LDFLAGS) -o $@ $(filter-out $(TIKTOKEN_GENERATED_HEADER), $(filter-out deps, $^))
	@echo "✅ Test link complete"

# Build and run the main tests
test: $(BUILD_DIR)/dirdoc_test
	@echo "🚀 Running main tests..."
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
	@echo "  all             - Build the dirdoc application (downloads deps, generates header)"
	@echo "  deps            - Download and set up dependencies (Cosmopolitan)"
	@echo "  test            - Build and run the test suite"
	@echo "  clean           - Remove build artifacts (tools and generated files in build dir)"
	@echo "  super_clean     - Remove build artifacts and dependencies (complete cleanup)"
	@echo "  help            - Show this help message"