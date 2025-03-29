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
