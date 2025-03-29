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
