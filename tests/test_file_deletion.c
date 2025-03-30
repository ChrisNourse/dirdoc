/* Tests for deletion of existing files before generating documentation. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#define MAX_PATH_LEN 4096

#include "../src/dirdoc.h"

/* Helper function to create a test output file */
static int create_test_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    
    fprintf(f, "%s", content);
    fclose(f);
    return 1;
}

/* Test that an existing file gets deleted before documentation is generated */
int test_delete_existing_file() {
    const char *test_dir = "./tests/test_data";
    const char *output_file = "./tests/test_data/test_output.md";
    
    // Create directory if it doesn't exist
    mkdir("./tests", 0755);
    mkdir(test_dir, 0755);
    
    // Create a dummy documentation file in the directory to be documented
    create_test_file("./tests/test_data/sample.txt", "This is a sample file to document");
    
    // Create a file to test deletion
    assert(create_test_file(output_file, "# This is existing content that should be deleted\n"));
    assert(access(output_file, F_OK) == 0); // File should exist
    
    // Run document_directory which should delete the existing file first
    int result = document_directory(test_dir, output_file, 0);
    
    // Check that the function completed successfully
    assert(result == 0);
    
    // The file might have been split, so check for either the original file
    // or a split version (with _part1 suffix)
    char split_file[MAX_PATH_LEN];
    snprintf(split_file, sizeof(split_file), "%s_part1.md", test_dir);
    
    if (access(output_file, F_OK) != 0 && access(split_file, F_OK) != 0) {
        printf("Error: Neither original output file (%s) nor split file (%s) exists\n", 
               output_file, split_file);
        assert(0);  // Force failure with a clearer message
    }
    
    // Open and check that the content is not the original content
    // Check which file to open
    char *file_to_check = NULL;
    if (access(output_file, F_OK) == 0) {
        file_to_check = strdup(output_file);
    } else {
        // Try the split file
        char split_file[MAX_PATH_LEN];
        snprintf(split_file, sizeof(split_file), "%s_part1.md", test_dir);
        if (access(split_file, F_OK) == 0) {
            file_to_check = strdup(split_file);
        }
    }
    
    assert(file_to_check != NULL);
    FILE *f = fopen(file_to_check, "r");
    assert(f != NULL);
    
    char buffer[100];
    fgets(buffer, sizeof(buffer), f);
    fclose(f);
    free(file_to_check);
    
    // Check that the content has changed and is not the original text
    assert(strstr(buffer, "This is existing content") == NULL);
    
    // Clean up - check which file exists and remove it
    if (access(output_file, F_OK) == 0) {
        remove(output_file);
    } else {
        // Try to remove the split file if it exists
        char split_file[MAX_PATH_LEN];
        snprintf(split_file, sizeof(split_file), "%s_part1.md", test_dir);
        if (access(split_file, F_OK) == 0) {
            remove(split_file);
        }
    }
    remove("./tests/test_data/sample.txt");
    
    printf("✓ test_delete_existing_file passed\n");
    return 0;
}

// Function to run all file deletion tests
int run_file_deletion_tests(void) {
    int errors = 0;
    
    errors += test_delete_existing_file();
    
    if (errors == 0) {
        printf("All file deletion tests passed!\n");
        return 0;
    } else {
        printf("Failed %d tests\n", errors);
        return 1;
    }
}

/* Only define main when explicitly building this file as standalone */
#ifdef FILE_DELETION_STANDALONE
int main(void) {
    return run_file_deletion_tests();
}
#endif
