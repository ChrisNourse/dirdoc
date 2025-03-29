# API Documentation

This document provides documentation for the Sample Project API.

## Functions

### print_info()

```c
void print_info(const char* message);
```

Prints an information message with the current timestamp.

**Parameters:**
- `message`: The message to be displayed

**Example:**
```c
print_info("Application started");
```

### is_prime()

```c
bool is_prime(int n);
```

Determines if a number is prime.

**Parameters:**
- `n`: The number to check

**Returns:**
- `true` if the number is prime
- `false` otherwise

**Example:**
```c
if (is_prime(17)) {
    printf("17 is prime\n");
}
```

### factorial()

```c
long factorial(int n);
```

Calculates the factorial of a number.

**Parameters:**
- `n`: The number to calculate factorial for

**Returns:**
- The factorial value (n!)

**Example:**
```c
long result = factorial(5); // result = 120
```
