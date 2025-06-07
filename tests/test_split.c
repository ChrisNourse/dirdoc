#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>
#include <stdbool.h>
#include <unistd.h> // For access()
#include "dirdoc.h"
#include "writer.h"
#include "scanner.h"
#include "gitignore.h"
#include "stats.h"

// Forward declaration from writer.c for direct unit testing
size_t find_split_points(const char *content, size_t limit, size_t *split_points, size_t max_splits);

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

    // Create output path in the tmp directory
    char output_path[256];
    snprintf(output_path, sizeof(output_path), "%s_documentation.md", test_dir);
    
    // Run the documentation generation with the specified output path
    int result = document_directory(test_dir, output_path, SPLIT_OUTPUT);
    assert(result == 0);

    // Check that the documented file content is preserved in one part
    char output_base[256];
    snprintf(output_base, sizeof(output_base), "%s_documentation", test_dir);

    bool found = false;
    char part_path[256];
    for (int i = 1; i <= 60; i++) {
        snprintf(part_path, sizeof(part_path), "%s_part%d.md", output_base, i);
        if (access(part_path, F_OK) != 0) {
            continue;
        }

        FILE *pf = fopen(part_path, "r");
        assert(pf != NULL);
        fseek(pf, 0, SEEK_END);
        long size = ftell(pf);
        fseek(pf, 0, SEEK_SET);
        char *content = malloc(size + 1);
        fread(content, 1, size, pf);
        content[size] = '\0';
        fclose(pf);

        if (strstr(content, "Documented File")) {
            // The documented file should only appear in one part
            assert(!found);
            assert(strstr(content, "Content of documented file.") != NULL);
            found = true;
        }

        free(content);
    }

    assert(found);
    printf("✓ Documented file content was preserved intact\n");

    // Clean up any split files that might have been created
    for (int i = 1; i <= 60; i++) {
        snprintf(part_path, sizeof(part_path), "%s_part%d.md", output_base, i);
        if (access(part_path, F_OK) == 0) {
            remove(part_path);
        }
    }

    // Cleanup
    remove(doc_path);
    remove(large_path);
    rmdir(test_dir);

    printf("✔ test_smart_split passed\n");
}

/**
 * @brief Ensure splitting only triggers on the exact UTF-8 marker.
 */
void test_split_marker_length() {
    // Build a long prefix so the false marker is inside the search window
    char prefix[70];
    memset(prefix, 'A', sizeof(prefix) - 1);
    prefix[sizeof(prefix) - 1] = '\0';

    char content[512];
    snprintf(content, sizeof(content),
             "%s\n### 📝 Wrong marker\nSome filler text to extend length\n\n### 📄 Correct marker\nEnd\n",
             prefix);

    size_t points[2];
    size_t splits = find_split_points(content, 120, points, 2);

    const char *wrong = strstr(content, "\n### 📝");
    assert(wrong != NULL);
    assert(splits == 1);
    assert(points[0] > (size_t)(wrong - content + 1));

    printf("✔ test_split_marker_length passed\n");
}

// Run function for the split tests
void run_split_tests() {
    printf("Running split tests...\n");
    test_split_marker_length();
    test_smart_split();
    printf("All split tests passed!\n");
}

