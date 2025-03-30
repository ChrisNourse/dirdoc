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
CFLAGS = -g -O2 -Ideps/cosmocc/include $(TIKTOKEN_INCLUDE)
LDFLAGS = $(TIKTOKEN_LDFLAGS)

# Tiktoken integration (fetching data from repo)
TIKTOKEN_REPO_URL = https://github.com/openai/tiktoken.git
TIKTOKEN_DIR      = $(DEPS_DIR)/tiktoken
TIKTOKEN_ENCODER_NAME = cl100k_base # Or change to gpt2, r50k_base, p50k_base, p50k_edit, etc.
TIKTOKEN_DATA_FILE = $(TIKTOKEN_DIR)/tiktoken/encoders/$(TIKTOKEN_ENCODER_NAME).tiktoken
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

# C++ compiler
CXX = $(DEPS_DIR)/cosmocc/bin/cosmoc++

# Ensure dirdoc.o is linked last (it provides get_default_output)
DIRDOC_OBJ = $(BUILD_DIR)/dirdoc.o
# Main C++ sources
MAIN_CPP_SOURCES = $(SRC_DIR)/tiktoken_cpp.cpp
MAIN_CPP_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(MAIN_CPP_SOURCES))
# All C++ objects (main + tools)
ALL_CPP_OBJECTS = $(MAIN_CPP_OBJECTS) $(TIKTOKEN_GEN_TOOL_OBJ)
# Objects for the final dirdoc executable
DIRDOC_LINK_OBJS = $(filter-out $(DIRDOC_OBJ), $(OBJECTS)) $(MAIN_CPP_OBJECTS)


.PHONY: all clean super_clean deps deps_cosmo deps_tiktoken help test build_temp clean_temp samples test_tiktoken tools

# Target for building tools
tools: $(TIKTOKEN_GEN_TOOL_EXE)

# Main build target depends on the final binary
all: $(BUILD_DIR)/dirdoc
	@echo "✅ Build completed successfully"
	@echo "📍 Binary location: $(BUILD_DIR)/dirdoc"
	@echo "🚀 Run ./$(BUILD_DIR)/dirdoc --help for usage"

# Combined dependencies target
deps: deps_cosmo deps_tiktoken

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

# Tiktoken dependency target: clone repo
deps_tiktoken: $(TIKTOKEN_DIR)/.git

$(TIKTOKEN_DIR)/.git:
	@echo "⏳ Checking tiktoken repository..."
	@if [ ! -d "$(TIKTOKEN_DIR)" ]; then \
		echo "📦 Cloning tiktoken repository from $(TIKTOKEN_REPO_URL)..."; \
		git clone --depth 1 $(TIKTOKEN_REPO_URL) $(TIKTOKEN_DIR); \
		echo "✅ Tiktoken repository cloned."; \
	else \
		echo "✅ Tiktoken repository already exists, skipping clone."; \
		echo "ℹ️  Consider 'rm -rf $(TIKTOKEN_DIR) && make deps_tiktoken' to re-clone if needed."; \
	fi
	@touch $@ # Update timestamp for Make

# Target to generate the tiktoken C header from the cloned repo data
# Requires the compiled C++ generator tool.
$(TIKTOKEN_GENERATED_HEADER): $(TIKTOKEN_GEN_TOOL_EXE) $(TIKTOKEN_DATA_FILE) | $(BUILD_DIR) deps_tiktoken
	@echo "⏳ Generating tiktoken C header $(TIKTOKEN_GENERATED_HEADER)..."
	@echo "⚙️ Running $(TIKTOKEN_GEN_TOOL_EXE)..."
	./$(TIKTOKEN_GEN_TOOL_EXE) $(TIKTOKEN_DATA_FILE) $@ $(TIKTOKEN_ENCODER_NAME)
	@echo "✅ Tiktoken C header generation step finished."

# Rule to build the generator tool executable
$(TIKTOKEN_GEN_TOOL_EXE): $(TIKTOKEN_GEN_TOOL_OBJ) | $(BUILD_DIR) deps_cosmo
	@echo "⏳ Linking generator tool $(TIKTOKEN_GEN_TOOL_EXE)..."
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "✅ Generator tool linked."

# Rule to compile the generator tool object file
$(TIKTOKEN_GEN_TOOL_OBJ): $(TIKTOKEN_GEN_TOOL_SRC) | $(BUILD_DIR) deps_cosmo
	@echo "⏳ Compiling generator tool $<..."
	$(CXX) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@ # Include src for base64.h
	@echo "✅ Generator tool compiled."

# Explicit rule stating that the data file depends on the repo being cloned.
$(TIKTOKEN_DATA_FILE): deps_tiktoken
	@# This rule ensures the dependency is tracked. The file is created by git clone.
	@# We add a check here to make sure the file exists after cloning.
	@# Use the variable explicitly instead of $@ to avoid potential expansion issues.
	@if [ ! -f "$(TIKTOKEN_DATA_FILE)" ]; then \
		echo "❌ Error: Tiktoken data file '$(TIKTOKEN_DATA_FILE)' not found after running deps_tiktoken."; \
		echo "ℹ️  Check the clone step or the path in the Makefile."; \
		exit 1; \
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

# Compile each source file into an object file.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile main C++ files, ensuring tiktoken_cpp.o depends on the generated header
$(BUILD_DIR)/tiktoken_cpp.o: $(SRC_DIR)/tiktoken_cpp.cpp $(TIKTOKEN_GENERATED_HEADER) | $(BUILD_DIR) deps_tiktoken
	$(CXX) $(CFLAGS) -c $(SRC_DIR)/tiktoken_cpp.cpp -o $@

# Link all object files together for the main executable, ensuring dirdoc.o is last.
# Make sure the generated header exists before linking.
$(BUILD_DIR)/dirdoc: $(DIRDOC_LINK_OBJS) $(DIRDOC_OBJ) $(TIKTOKEN_GENERATED_HEADER) | deps
	@echo "⏳ Linking dirdoc..."
	$(CXX) $(LDFLAGS) -o $@ $(DIRDOC_LINK_OBJS) $(DIRDOC_OBJ)
	@echo "✅ Build complete"

# Update test build dependencies (uses MAIN_CPP_OBJECTS)
$(BUILD_DIR)/dirdoc_test: $(wildcard $(SRC_DIR)/*.c) $(wildcard $(TEST_DIR)/*.c) $(MAIN_CPP_OBJECTS) $(TIKTOKEN_GENERATED_HEADER) | $(BUILD_DIR) deps
	@echo "⏳ Building tests..."
	$(CXX) $(CFLAGS) -DUNIT_TEST -I. -I$(SRC_DIR) -I$(TEST_DIR) -Ideps/cosmocc/include -o $@ $(filter-out $(TIKTOKEN_GENERATED_HEADER), $^) $(LDFLAGS)
	@echo "✅ Test build complete"

# Update temp test build dependencies (uses MAIN_CPP_OBJECTS)
$(BUILD_DIR)/temp_test: $(wildcard $(SRC_DIR)/*.c) $(wildcard $(TEST_DIR)/*.c) $(MAIN_CPP_OBJECTS) $(TIKTOKEN_GENERATED_HEADER) | $(BUILD_DIR) deps
	@echo "⏳ Building temp test binary..."
	$(CXX) $(CFLAGS) -DUNIT_TEST -DINSPECT_TEMP -I. -I$(SRC_DIR) -I$(TEST_DIR) -Ideps/cosmocc/include -o $@ $(filter-out $(TIKTOKEN_GENERATED_HEADER), $^) $(LDFLAGS)
	@echo "✅ Temp test build complete"

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

# Update tiktoken test build dependencies (needs tiktoken_cpp.o)
$(BUILD_DIR)/test_tiktoken: $(SRC_DIR)/tiktoken.c $(BUILD_DIR)/tiktoken_cpp.o $(SRC_DIR)/stats.c tests/test_tiktoken.c $(TIKTOKEN_GENERATED_HEADER) | $(BUILD_DIR) deps
	@echo "⏳ Building tiktoken tests..."
	$(CXX) $(CFLAGS) -DUNIT_TEST -I. -I$(SRC_DIR) -I$(TEST_DIR) -Ideps/cosmocc/include -o $@ $(filter-out $(TIKTOKEN_GENERATED_HEADER) $(BUILD_DIR)/tiktoken_cpp.o, $^) $(BUILD_DIR)/tiktoken_cpp.o $(LDFLAGS)
	@echo "✅ Tiktoken test build complete"

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
	@echo "  all         - Build the dirdoc application (clones deps, generates header)"
	@echo "  deps        - Download/clone and set up dependencies (Cosmopolitan & Tiktoken repo)"
	@echo "  deps_cosmo  - Download and set up Cosmopolitan dependency"
	@echo "  deps_tiktoken - Clone the Tiktoken repository"
	@echo "  tools       - Build helper tools (e.g., tiktoken data generator)"
	@echo "  $(TIKTOKEN_GENERATED_HEADER) - Generate the tiktoken C header (requires compiled C++ tool and cloned repo)"
	@echo "  clean       - Remove build artifacts (including tools)"
	@echo "  super_clean - Remove build artifacts and dependencies"
	@echo "  test        - Build and run the test suite"
	@echo "  build_temp  - Build the test binary for generating temp test files (without auto-cleanup)"
	@echo "  clean_temp  - Remove all temporary test files from the 'tmp' directory"
	@echo "  samples     - Create a sample project structure to demonstrate dirdoc"
	@echo "  help        - Show this help message"
	
