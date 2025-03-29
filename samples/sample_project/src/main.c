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
