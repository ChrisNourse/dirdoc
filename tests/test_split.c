#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dirdoc.h"
#include "writer.h"
#include "scanner.h"
#include "gitignore.h"
#include "stats.h"
#include "tests.h"  // Assuming a common header for tests

/**
 * @brief Test that verifies smart splitting preserves documented files.
 */
void test_smart_split() {
    // Setup test directory with a documented file and other files
    const char *test_dir = "tmp/test_smart_split";
    mkdir(test_dir, 0755);

    // Create a documented file
    const char *doc_file = "docs.md";
    char doc_path[256];
    snprintf(doc_path, sizeof(doc_path), "%s/%s", test_dir, doc_file);
    FILE *f = fopen(doc_path, "w");
    fprintf(f, "### Documented File\nContent of documented file.");
    fclose(f);

    // Create another file to be split
    const char *large_file = "large.txt";
    char large_path[256];
    snprintf(large_path, sizeof(large_path), "%s/%s", test_dir, large_file);
    f = fopen(large_path, "w");
    for (int i = 0; i < 10000; i++) {
        fprintf(f, "Line %d: This is a test line to create a large file.\n", i);
    }
    fclose(f);

    // Set split options to enable splitting
    set_split_options(1, 0.01); // 0.01 MB limit for testing

    // Run the documentation generation
    int result = document_directory(test_dir, NULL, SPLIT_OUTPUT);
    assert(result == 0);

    // Check that the documented file was not split
    char expected_doc_split_path[256];
    snprintf(expected_doc_split_path, sizeof(expected_doc_split_path), "%s_part1.md", doc_path);
    // For the documented file, it should not be split, so check only the original exists
    FILE *doc_split = fopen(expected_doc_split_path, "w");
    if (doc_split) {
        fclose(doc_split);
        assert(0 && "Documented file should not be split");
    }

    // Cleanup
    remove(doc_path);
    remove(large_path);
    rmdir(test_dir);

    printf("✔ test_smart_split passed\n");
}

int main() {
    printf("Running smart split tests...\n");
    test_smart_split();
    printf("All smart split tests passed!\n");
    return 0;
}
