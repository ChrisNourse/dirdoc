# Documentation Summary

The output is a Markdown document summarizing a directory’s structure and file contents. It begins with token and size statistics, followed by a hierarchical view of the directory layout. For each file (unless omitted in structure-only mode), its contents are included in fenced code blocks with optional language annotations and metadata like file size, forming a complete, self-contained reference.

Token Size: 3289

# Directory Documentation: 

## Structure

```
📄 example_.gitignore
📄 example_Makefile
📄 example_README.md
📄 example_project_documentation.md
📁 src/
├── 📄 example_main.c
├── 📄 example_utils.c
├── 📄 example_utils.h
📁 tests/
├── 📄 example_test_utils.c
```

## Contents

### 📄 example_.gitignore

```
# Build directories
bin/
obj/

# Editor files
.vscode/
.idea/
*.swp
*~

# Compiled files
*.o
*.out
*.exe

# Debug files
*.dSYM/
*.su
*.idb
*.pdb

# OS specific files
.DS_Store
Thumbs.db
```

### 📄 example_Makefile

```

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
LDFLAGS = -lm

SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Source files and object files
SRCS = $(SRCDIR)/main.c $(SRCDIR)/utils.c
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Target executable
TARGET = $(BINDIR)/sample_app

# Default target
all: dirs $(TARGET)

# Create directories
dirs:
	mkdir -p $(BINDIR) $(OBJDIR)

# Compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Clean up
clean:
	rm -rf $(BINDIR) $(OBJDIR)

# Run the application
run: all
	$(TARGET)

.PHONY: all dirs clean run
```

### 📄 example_README.md

```markdown
# Sample Project

This is a sample project to demonstrate the capabilities of dirdoc. It contains different file types, directory structures, and file contents that would be typical in a real software project.

## Features

- Multiple directories and file types
- A mix of code and documentation
- Examples of binary and text files
- Basic project structure

## Usage

This is purely a demonstration project for dirdoc to document.
```

### 📄 example_project_documentation.md

```markdown
# Sample Project

This is a sample project to demonstrate the capabilities of dirdoc. It contains different file types, directory structures, and file contents that would be typical in a real software project.

## Features

- Multiple directories and file types
- A mix of code and documentation
- Examples of binary and text files
- Basic project structure

## Usage

This is purely a demonstration project for dirdoc to document.
```

### 📄 src/example_main.c

```c
/**
 * Main.c - Entry point for the sample application
 * 
 * This file demonstrates a simple C application with comments,
 * function declarations, and basic program structure.
 */

#include <stdio.h>
#include "utils.h"

// Application version
const char* VERSION = "1.0.0";

/**
 * Main entry point for the application
 */
int main(int argc, char *argv[]) {
    printf("Sample Application v%s\n", VERSION);
    
    // Call our utility function
    print_info("Application started");
    
    if (argc > 1) {
        printf("Arguments provided: %d\n", argc - 1);
        for (int i = 1; i < argc; i++) {
            printf("  Arg %d: %s\n", i, argv[i]);
        }
    } else {
        printf("No arguments provided\n");
    }
    
    // Example of some algorithmic code
    int numbers[] = {5, 2, 9, 1, 7, 4};
    int size = sizeof(numbers) / sizeof(numbers[0]);
    
    printf("Array before sorting: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Sort the array
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (numbers[j] > numbers[j+1]) {
                int temp = numbers[j];
                numbers[j] = numbers[j+1];
                numbers[j+1] = temp;
            }
        }
    }
    
    printf("Array after sorting: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    print_info("Application finished");
    return 0;
}
```

### 📄 src/example_utils.c

```c
/**
 * Utils.c - Implementation of utility functions
 */

#include "utils.h"
#include <stdio.h>
#include <time.h>
#include <math.h>

void print_info(const char* message) {
    time_t now;
    time(&now);
    
    char time_str[26];
    ctime_r(&now, time_str);
    
    // Remove newline character from time string
    time_str[24] = '\0';
    
    printf("[%s] INFO: %s\n", time_str, message);
}

bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    
    // Check if n is divisible by 2 or 3
    if (n % 2 == 0 || n % 3 == 0) return false;
    
    // Check using 6k +/- 1 rule
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    
    return true;
}

long factorial(int n) {
    if (n <= 0) return 1;
    
    long result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
    }
    
    return result;
}
```

### 📄 src/example_utils.h

```c
/**
 * Utils.h - Utility functions for the sample application
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

/**
 * Prints an information message with timestamp
 * 
 * @param message The message to print
 */
void print_info(const char* message);

/**
 * Checks if a number is prime
 * 
 * @param n The number to check
 * @return True if the number is prime, false otherwise
 */
bool is_prime(int n);

/**
 * Calculates factorial of a number
 * 
 * @param n The number to calculate factorial for
 * @return The factorial value
 */
long factorial(int n);

#endif /* UTILS_H */
```

### 📄 tests/example_test_utils.c

```c
/**
 * Test suite for utility functions
 */

#include <stdio.h>
#include <assert.h>
#include "../src/utils.h"

void test_is_prime() {
    // Test cases for is_prime function
    assert(is_prime(2) == true);
    assert(is_prime(3) == true);
    assert(is_prime(5) == true);
    assert(is_prime(7) == true);
    assert(is_prime(11) == true);
    
    assert(is_prime(1) == false);
    assert(is_prime(4) == false);
    assert(is_prime(6) == false);
    assert(is_prime(9) == false);
    assert(is_prime(15) == false);
    
    printf("All is_prime tests passed!\n");
}

void test_factorial() {
    // Test cases for factorial function
    assert(factorial(0) == 1);
    assert(factorial(1) == 1);
    assert(factorial(2) == 2);
    assert(factorial(3) == 6);
    assert(factorial(4) == 24);
    assert(factorial(5) == 120);
    
    printf("All factorial tests passed!\n");
}

int main() {
    printf("Running tests...\n");
    
    test_is_prime();
    test_factorial();
    
    printf("All tests passed successfully!\n");
    return 0;
}
```

