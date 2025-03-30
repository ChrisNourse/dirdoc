/* Tests for deletion of existing files before generating documentation. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

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
    
    // Create a file to test deletion
    assert(create_test_file(output_file, "# This is existing content that should be deleted\n"));
    assert(access(output_file, F_OK) == 0); // File should exist
    
    // Run document_directory which should delete the existing file first
    int result = document_directory(test_dir, output_file, 0);
    
    // Check that the function completed successfully
    assert(result == 0);
    
    // Check that the file exists (was recreated after deletion)
    assert(access(output_file, F_OK) == 0);
    
    // Open and check that the content is not the original content
    FILE *f = fopen(output_file, "r");
    assert(f != NULL);
    
    char buffer[100];
    fgets(buffer, sizeof(buffer), f);
    fclose(f);
    
    // Check that the content has changed and is not the original text
    assert(strstr(buffer, "This is existing content") == NULL);
    
    // Clean up
    remove(output_file);
    
    printf("✓ test_delete_existing_file passed\n");
    return 0;
}

int main() {
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
