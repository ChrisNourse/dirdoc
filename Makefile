SHELL = /bin/bash

CC       = gcc
CFLAGS   = -g -O -static -fno-pie -no-pie -mno-red-zone -nostdlib -nostdinc
LDFLAGS  = -Wl,--oformat=binary \
           -Wl,--gc-sections \
           -Wl,-z,max-page-size=0x1000 \
           -Wl,-T deps/cosmo/ape.lds

SRC_DIR   = src
DEPS_DIR  = deps
BUILD_DIR = build

COSMO_ZIP_URL = https://github.com/jart/cosmopolitan/releases/download/2.1/cosmopolitan-amalgamation-2.1.zip
COSMO_ZIP     = cosmopolitan.zip

.PHONY: all clean test deps

all: deps $(BUILD_DIR)/dirdoc
	@echo "✅ Build completed successfully"
	@echo "📍 Binary location: $(BUILD_DIR)/dirdoc"
	@echo "🚀 Run ./$(BUILD_DIR)/dirdoc --help for usage"

deps:
	@if [ ! -f "$(DEPS_DIR)/cosmo/cosmopolitan.a" ]; then \
		mkdir -p "$(DEPS_DIR)/cosmo" && \
		echo "📦 Fetching Cosmopolitan dependencies..." && \
		if command -v curl >/dev/null 2>&1; then \
			echo "   ⬇️  Downloading with curl..." && \
			curl -fSL "$(COSMO_ZIP_URL)" -o "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" || \
			{ echo "❌ Download failed"; exit 1; }; \
		elif command -v wget >/dev/null 2>&1; then \
			echo "   ⬇️  Downloading with wget..." && \
			wget -q --show-progress -O "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" "$(COSMO_ZIP_URL)" || \
			{ echo "❌ Download failed"; exit 1; }; \
		else \
			echo "❌ Error: Neither curl nor wget found"; \
			exit 1; \
		fi && \
		echo "   📂 Unpacking..." && \
		if command -v unzip >/dev/null 2>&1; then \
			unzip -q "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" -d "$(DEPS_DIR)/cosmo" || \
			{ echo "❌ Unpacking failed"; exit 1; }; \
		elif command -v 7z >/dev/null 2>&1; then \
			7z x -y "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" -o"$(DEPS_DIR)/cosmo" >/dev/null || \
			{ echo "❌ Unpacking failed"; exit 1; }; \
		else \
			echo "❌ Error: Neither unzip nor 7z found"; \
			exit 1; \
		fi && \
		rm "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" && \
		echo "✅ Dependencies installed successfully"; \
	else \
		echo "✅ Dependencies already installed"; \
	fi

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@echo "📁 Created build directory"

$(BUILD_DIR)/dirdoc: $(SRC_DIR)/dirdoc.c $(SRC_DIR)/dirdoc_impl.c | $(BUILD_DIR) deps
	@echo "🔨 Building dirdoc..."
	@$(CC) $(CFLAGS) \
		-I$(DEPS_DIR)/cosmo \
		-o $@ $^ $(LDFLAGS) \
		$(DEPS_DIR)/cosmo/crt.o \
		$(DEPS_DIR)/cosmo/ape.o \
		$(DEPS_DIR)/cosmo/cosmopolitan.a \
		|| { echo "❌ Build failed"; exit 1; }

test: $(SRC_DIR)/test_dirdoc.c $(SRC_DIR)/dirdoc_impl.c
	@echo "🧪 Running tests..."
	@$(CC) -g -o $(BUILD_DIR)/test_dirdoc $^ -I$(DEPS_DIR)/greatest -I$(SRC_DIR) || \
		{ echo "❌ Test build failed"; exit 1; }
	@./$(BUILD_DIR)/test_dirdoc || { echo "❌ Tests failed"; exit 1; }
	@echo "✅ All tests passed"

clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "✅ Clean complete"