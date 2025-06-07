#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reconstruct.h"

/* Functions from test_dirdoc.c */
char *create_temp_dir();
int remove_directory_recursive(const char *path);

void test_reconstruct_basic() {
    const char *md = "example_project/example_project_documentation.md";
    char *out_dir = create_temp_dir();
    int ret = reconstruct_from_markdown(md, out_dir);
    assert(ret == 0);

    char path[512];
    snprintf(path, sizeof(path), "%s/src/example_main.c", out_dir);
    FILE *f = fopen(path, "r");
    assert(f != NULL);
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    assert(strstr(line, "/*") != NULL);

    remove_directory_recursive(out_dir);
    free(out_dir);
    printf("✔ test_reconstruct_basic passed\n");
}

void test_reconstruct_binary_placeholder() {
    const char *markdown =
        "# Directory Documentation:\n\n"
        "### \xF0\x9F\x93\x84 bin/file.bin\n\n"
        "```\n"
        "*Binary file*\n"
        "```\n";

    char *out_dir = create_temp_dir();
    char md_path[512];
    snprintf(md_path, sizeof(md_path), "%s/doc.md", out_dir);
    FILE *md = fopen(md_path, "w");
    assert(md != NULL);
    fputs(markdown, md);
    fclose(md);

    int ret = reconstruct_from_markdown(md_path, out_dir);
    assert(ret == 0);

    char path[512];
    snprintf(path, sizeof(path), "%s/bin/file.bin", out_dir);
    FILE *f = fopen(path, "r");
    assert(f != NULL);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    assert(size == 0);

    remove_directory_recursive(out_dir);
    free(out_dir);
    printf("✔ test_reconstruct_binary_placeholder passed\n");
}

void run_reconstruct_tests() {
    printf("Running reconstruct tests...\n");
    test_reconstruct_basic();
    test_reconstruct_binary_placeholder();
    printf("All reconstruct tests passed!\n");
}
