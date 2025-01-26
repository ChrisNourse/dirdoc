// test_dirdoc.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "greatest.h"
#include "dirdoc.h"

// Test utilities
static char test_dir[MAX_PATH_LEN];
static char output_file[MAX_PATH_LEN];

static void setup_test() {
    // Create temporary test directory
    sprintf(test_dir, "/tmp/dirdoc_test_XXXXXX");
    mkdtemp(test_dir);
    
    // Set output file path
    sprintf(output_file, "%s/output.md", test_dir);
}

static void teardown_test() {
    char cmd[MAX_PATH_LEN];
    sprintf(cmd, "rm -rf %s", test_dir);
    system(cmd);
}

static void create_test_file(const char *path, const char *content) {
    char full_path[MAX_PATH_LEN];
    sprintf(full_path, "%s/%s", test_dir, path);
    
    // Create directories if needed
    char *last_slash = strrchr(full_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(full_path, 0755);
        *last_slash = '/';
    }
    
    FILE *f = fopen(full_path, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

static void create_test_binary(const char *path, size_t size) {
    char full_path[MAX_PATH_LEN];
    sprintf(full_path, "%s/%s", test_dir, path);
    
    FILE *f = fopen(full_path, "wb");
    if (f) {
        unsigned char byte = 0;
        for (size_t i = 0; i < size; i++) {
            fwrite(&byte, 1, 1, f);
        }
        fclose(f);
    }
}

// Tests
TEST test_binary_detection(void) {
    create_test_file("text.txt", "Hello World");
    create_test_binary("binary.bin", 1024);
    
    char path[MAX_PATH_LEN];
    sprintf(path, "%s/text.txt", test_dir);
    ASSERT_FALSE(is_binary_file(path));
    
    sprintf(path, "%s/binary.bin", test_dir);
    ASSERT_TRUE(is_binary_file(path));
    
    PASS();
}

TEST test_file_size(void) {
    create_test_file("small.txt", "Test");
    create_test_binary("large.bin", 1024 * 1024); // 1MB
    
    char path[MAX_PATH_LEN];
    sprintf(path, "%s/small.txt", test_dir);
    ASSERT_STR_EQ("4.00 B", get_file_size(path));
    
    sprintf(path, "%s/large.bin", test_dir);
    ASSERT_STR_EQ("1.00 MB", get_file_size(path));
    
    PASS();
}

TEST test_directory_structure(void) {
    // Create test structure
    create_test_file("root.txt", "Root file");
    create_test_file("dir1/file1.txt", "File 1");
    create_test_file("dir1/subdir/file2.txt", "File 2");
    
    // Document directory
    document_directory(test_dir, output_file);
    
    // Check output file exists
    ASSERT(access(output_file, F_OK) != -1);
    
    // Verify content
    FILE *f = fopen(output_file, "r");
    char line[1024];
    bool found_structure = false;
    bool found_dir1 = false;
    bool found_subdir = false;
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "## Structure")) found_structure = true;
        if (strstr(line, "📁 dir1")) found_dir1 = true;
        if (strstr(line, "📁 subdir")) found_subdir = true;
    }
    fclose(f);
    
    ASSERT(found_structure);
    ASSERT(found_dir1);
    ASSERT(found_subdir);
    
    PASS();
}

TEST test_file_contents(void) {
    const char *test_content = "Test content\nWith multiple lines\n";
    create_test_file("test.txt", test_content);
    
    document_directory(test_dir, output_file);
    
    FILE *f = fopen(output_file, "r");
    char content[1024] = {0};
    bool found_fence = false;
    bool found_content = false;
    
    while (fgets(content, sizeof(content), f)) {
        if (strcmp(content, FENCE"\n") == 0) found_fence = true;
        if (strstr(content, test_content)) found_content = true;
    }
    fclose(f);
    
    ASSERT(found_fence);
    ASSERT(found_content);
    
    PASS();
}

SUITE(dirdoc_suite) {
    SET_SETUP(setup_test);
    SET_TEARDOWN(teardown_test);
    
    RUN_TEST(test_binary_detection);
    RUN_TEST(test_file_size);
    RUN_TEST(test_directory_structure);
    RUN_TEST(test_file_contents);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(dirdoc_suite);
    GREATEST_MAIN_END();
}
