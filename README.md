# DirDoc

Command-line tool that generates comprehensive Markdown documentation of directory structures. Captures full text file contents, binary file metadata, and provides token statistics.

## Installation

Download latest release: [dirdoc.com](releases/latest)

## Usage

```bash
./dirdoc.com /path/to/directory [output.md]
```

## Output Example

````````markdown
# Directory Documentation: example

## Structure
- 📁 src/
  - 📄 main.c
  - 📄 README.md
- 📁 data/
  - 📄 config.json

## Contents

### 📄 src/main.c
```c
int main() {
    printf("Hello World");
    return 0;
}
```

### 📄 src/README.md
````markdown
# Component

```c
// Example code block
void test() {
}
```

## Usage
```bash
make build
```

### 📄 data/config.json
```json
{
    "version": "1.0"
}
```

## Document Statistics
- Characters: 1234
- Words: 234
- Tokens: 345
````````

---

## Developer Guide

### Build Requirements

#### Windows
1. I gave up on trying to set this up

#### Ubuntu/Debian
```bash
sudo apt install build-essential git
```

#### macOS
```bash
xcode-select --install
brew install gcc
```

### Project Structure
```
dirdoc/
├── src/              # Source files
│   ├── dirdoc.c      # Main entry point
│   ├── dirdoc.h      # Public API
│   ├── dirdoc_impl.c # Implementation
│   └── test_dirdoc.c # Tests
├── deps/            # Dependencies
│   ├── cosmo/       # Cosmopolitan APE
│   └── greatest/    # Test framework
├── build/          # Build artifacts
├── Makefile
└── README.md
```

### Building

```bash
# Build executable
make

# Run tests
make test

# Clean build files
make clean
```

### Features
- Cross-platform via Actually Portable Executable (APE)
- Smart binary file detection with size and hash
- Token, word, and character statistics
- Adaptive Markdown fence handling for nested code blocks
- Complete directory structure traversal
- Full file content preservation

### Testing
Uses Greatest test framework (header-only). All tests are in `src/test_dirdoc.c`.

Test coverage includes:
- Binary file detection
- File size calculation
- Directory traversal
- File content handling
- Token counting
- Nested markdown handling

## License

MIT License

## Credit

Built with [Cosmopolitan](https://github.com/jart/cosmopolitan) for cross-platform support.
