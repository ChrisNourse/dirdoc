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
