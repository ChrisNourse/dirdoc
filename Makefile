SHELL = /bin/bash

CC       = gcc
CFLAGS = -g -O -static -fno-pie -no-pie -mno-red-zone -nostdlib -nostdinc
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

# A single-line shell script to avoid multiline issues
deps:
	@if [ ! -f "$(DEPS_DIR)/cosmo/cosmopolitan.a" ]; then mkdir -p "$(DEPS_DIR)/cosmo"; echo "=== Fetching prebuilt Cosmopolitan release ==="; if command -v curl >/dev/null 2>&1; then echo "Downloading with curl..."; curl -fSL "$(COSMO_ZIP_URL)" -o "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)"; elif command -v wget >/dev/null 2>&1; then echo "Downloading with wget..."; wget -O "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" "$(COSMO_ZIP_URL)"; else echo "Error: Neither 'curl' nor 'wget' is installed."; exit 1; fi; if command -v unzip >/dev/null 2>&1; then echo "Unpacking with unzip..."; unzip "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" -d "$(DEPS_DIR)/cosmo"; elif command -v 7z >/dev/null 2>&1; then echo "Unpacking with 7z..."; 7z x "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)" -o"$(DEPS_DIR)/cosmo"; else echo "Error: Neither 'unzip' nor '7z' is installed."; exit 1; fi; rm "$(DEPS_DIR)/cosmo/$(COSMO_ZIP)"; echo "Cosmopolitan release unpacked."; fi

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/dirdoc: $(SRC_DIR)/dirdoc.c $(SRC_DIR)/dirdoc_impl.c | $(BUILD_DIR) deps
	$(CC) $(CFLAGS) \
		-I$(DEPS_DIR)/cosmo \
		-o $@ $^ $(LDFLAGS) \
		$(DEPS_DIR)/cosmo/crt.o \
		$(DEPS_DIR)/cosmo/ape.o \
		$(DEPS_DIR)/cosmo/cosmopolitan.a

test: $(SRC_DIR)/test_dirdoc.c $(SRC_DIR)/dirdoc_impl.c
	$(CC) -g -o $(BUILD_DIR)/test_dirdoc $^ -I$(DEPS_DIR)/greatest -I$(SRC_DIR)
	./$(BUILD_DIR)/test_dirdoc

clean:
	rm -rf $(BUILD_DIR)
