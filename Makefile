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
		echo "📦 Fetching dependencies..." && \
		if command -v curl >/dev/null 2>&1; then \
			printf "⏳ Downloading..." && \
			curl -fSL "$(COSMO_ZIP_URL)" -o "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" && \
			printf "\r✅ Downloaded \n"; \
		elif command -v wget >/dev/null 2>&1; then \
			printf "⏳ Downloading..." && \
			wget -q "$(COSMO_ZIP_URL)" -O "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" && \
			printf "\r✅ Downloaded \n"; \
		else \
			echo "❌ Error: Neither curl nor wget found"; \
			exit 1; \
		fi && \
		printf "⏳ Unpacking..." && \
		if command -v unzip >/dev/null 2>&1; then \
			unzip -q "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" -d "$(DEPS_DIR)/cosmo" && \
			printf "\r✅ Unpacked  \n"; \
		elif command -v 7z >/dev/null 2>&1; then \
			7z x -y "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" -o"$(DEPS_DIR)/cosmo" >/dev/null && \
			printf "\r✅ Unpacked  \n"; \
		else \
			echo "❌ Error: Neither unzip nor 7z found"; \
			exit 1; \
		fi && \
		rm "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)"; \
	else \
		echo "✅ Dependencies ready"; \
	fi

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/dirdoc: $(SRC_DIR)/dirdoc.c $(SRC_DIR)/dirdoc_impl.c | $(BUILD_DIR) deps
	@printf "⏳ Building..." && \
	$(CC) $(CFLAGS) \
		-I$(DEPS_DIR)/cosmo \
		-o $@ $^ $(LDFLAGS) \
		$(DEPS_DIR)/cosmo/crt.o \
		$(DEPS_DIR)/cosmo/ape.o \
		$(DEPS_DIR)/cosmo/cosmopolitan.a && \
	printf "\r✅ Build complete\n"

test: $(SRC_DIR)/test_dirdoc.c $(SRC_DIR)/dirdoc_impl.c
	@printf "⏳ Building tests..." && \
	$(CC) -g -o $(BUILD_DIR)/test_dirdoc $^ -I$(DEPS_DIR)/greatest -I$(SRC_DIR) && \
	printf "\r✅ Tests built   \n" && \
	printf "⏳ Running tests..." && \
	./$(BUILD_DIR)/test_dirdoc && \
	printf "\r✅ Tests passed   \n"

clean:
	@printf "⏳ Cleaning..." && \
	rm -rf $(BUILD_DIR) && \
	printf "\r✅ Clean complete\n"