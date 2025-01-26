SHELL = /bin/bash

# Directories
SRC_DIR   = src
DEPS_DIR  = deps
BUILD_DIR = build

# Compiler and Flags
CC = $(DEPS_DIR)/cosmocc/bin/cosmocc
CFLAGS = -g -O2
LDFLAGS = 

# Cosmopolitan Libc 4.0.2 URLs
COSMO_ZIP_URL = https://github.com/jart/cosmopolitan/releases/download/4.0.2/cosmocc-4.0.2.zip
COSMO_ZIP     = cosmocc-4.0.2.zip

.PHONY: all clean super_clean deps help

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

$(BUILD_DIR)/dirdoc: $(SRC_DIR)/dirdoc.c $(SRC_DIR)/dirdoc_impl.c | $(BUILD_DIR) deps
	@echo "⏳ Building dirdoc..."
	$(CC) $(CFLAGS) -I$(DEPS_DIR)/cosmocc/include -o $@ $^ $(LDFLAGS)
	@echo "✅ Build complete"

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
	@echo "  help        - Show this help message"