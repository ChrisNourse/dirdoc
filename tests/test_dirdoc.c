#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>  // Add this line for basename declaration
#ifdef _WIN32
#include <windows.h>
#endif
#include <dirent.h>

#include "dirdoc.h"
#include "gitignore.h"
#include "scanner.h"
#include "stats.h"
#include "writer.h"

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 4096
#endif

/*
 * remove_directory_recursive:
 * Recursively deletes a directory and its contents.
 * Returns 0 on success, or a nonzero value on error.
 */
int remove_directory_recursive(const char *path) {
#ifdef _WIN32
    // Windows: use system command for simplicity
    char command[MAX_PATH_LEN];
    snprintf(command, sizeof(command), "rmdir /s /q \"%s\"", path);
    return system(command);
#else
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = 0;
    if (d) {
        struct dirent *p;
        while ((p = readdir(d)) != NULL) {
            // Skip the names "." and ".." as we don't want to recurse on them.
            if (strcmp(p->d_name, ".") == 0 || strcmp(p->d_name, "..") == 0)
                continue;
            char *buf;
            size_t len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);
            if (buf == NULL) {
                r = -1;
                break;
            }
            snprintf(buf, len, "%s/%s", path, p->d_name);
            struct stat statbuf;
            if (stat(buf, &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    r = remove_directory_recursive(buf);
                } else {
                    r = unlink(buf);
                }
            }
            free(buf);
            if (r != 0)
                break;
        }
        closedir(d);
    }
    if (r == 0)
        r = rmdir(path);
    return r;
#endif
}

/*
 * remove_empty_directory:
 * Checks if a directory is empty, and if so, removes it.
 * Returns 0 if removed successfully, 1 if not empty, or -1 on error.
 */
int remove_empty_directory(const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        perror("opendir");
        return -1;
    }
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }
    closedir(d);
    if (count == 0) {
        if (rmdir(path) == 0) {
            return 0;
        } else {
            perror("rmdir");
            return -1;
        }
    }
    return 1; // not empty
}

/* Helper function to create a temporary directory inside the local 'tmp' folder.
 * The returned pointer must be freed by the caller.
 */
char *create_temp_dir() {
    // Ensure the local "tmp" directory exists
    struct stat st = {0};
    if (stat("tmp", &st) == -1) {
        if (mkdir("tmp", 0755) != 0) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }

    // Template for temporary directory inside the local "tmp" folder
    char *template = strdup("tmp/dirdoc_test_XXXXXX");
    char *dir = mkdtemp(template);
    if (!dir) {
        perror("mkdtemp");
        free(template);
        exit(EXIT_FAILURE);
    }
    return dir;  // Note: caller is responsible for freeing the allocated string
}

/* Helper function to create a file with given contents. */
void create_file(const char *dir, const char *filename, const char *contents) {
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fputs(contents, f);
    fclose(f);
}

/* Test the get_default_output function */
#include <unistd.h>   // for getcwd()

void test_get_default_output() {
    // Test with an absolute path.
    const char *dummy_dir = "/some/path";
    char *output = get_default_output(dummy_dir);
    // The basename of "/some/path" is "path", so we expect "path_documentation.md"
    assert(strcmp(output, "path_documentation.md") == 0);
    free(output);

    // Test with the current directory "."
    char *cwd = getcwd(NULL, 0);
    assert(cwd != NULL);
    // Extract the actual folder name from the current working directory.
    char *base = basename(cwd);
    // Build the expected output: "<folder>_documentation.md"
    size_t expected_len = strlen(base) + strlen("_documentation.md") + 1;
    char *expected = malloc(expected_len);
    snprintf(expected, expected_len, "%s_documentation.md", base);

    output = get_default_output(".");
    assert(strcmp(output, expected) == 0);
    free(expected);
    free(output);
    free(cwd);

    printf("✔ test_get_default_output passed\n");
}

/* Test the get_language_from_extension function */
void test_get_language_from_extension() {
    assert(strcmp(get_language_from_extension("Makefile"), "make") == 0);
    assert(strcmp(get_language_from_extension("test.c"), "c") == 0);
    assert(strcmp(get_language_from_extension("header.h"), "c") == 0);
    assert(strcmp(get_language_from_extension("script.py"), "python") == 0);
    assert(strcmp(get_language_from_extension("unknown.xyz"), "") == 0);
    printf("✔ test_get_language_from_extension passed\n");
}

/* Test basic gitignore functionality:
 * Create a temporary directory with a .gitignore file and test load/match.
 */
void test_gitignore() {
    char *temp_dir = create_temp_dir();
    // Create a .gitignore file that ignores "ignoreme.txt"
    create_file(temp_dir, ".gitignore", "ignoreme.txt\n");
    
    GitignoreList gitignore;
    load_gitignore(temp_dir, &gitignore);
    // Path that should be ignored
    char path1[MAX_PATH_LEN];
    snprintf(path1, sizeof(path1), "ignoreme.txt");
    // Path that should not be ignored
    char path2[MAX_PATH_LEN];
    snprintf(path2, sizeof(path2), "donotignore.txt");
    
    assert(match_gitignore(path1, &gitignore) == 1);
    assert(match_gitignore(path2, &gitignore) == 0);
    free_gitignore(&gitignore);
    printf("✔ test_gitignore passed\n");
    
#ifndef INSPECT_TEMP
    if (remove_directory_recursive(temp_dir) == 0) {
        printf("Folder '%s' removed successfully.\n", temp_dir);
    } else {
        printf("Failed to remove folder '%s'.\n", temp_dir);
    }
#endif
    free(temp_dir);
}

/* Test gitignore wildcard patterns:
 * This test creates a .gitignore file with various wildcard patterns,
 * including extension wildcards, negations, anchored patterns, and directory patterns.
 */
void test_gitignore_wildcards() {
    char *temp_dir = create_temp_dir();
    // Create a .gitignore file with multiple patterns:
    //   - "*.log": ignore any file ending with .log
    //   - "!important.log": do not ignore important.log
    //   - "build/": ignore any file or directory under build (and build itself)
    //   - "temp*": ignore any file starting with "temp"
    //   - "!temp_keep.txt": do not ignore temp_keep.txt
    //   - "/config/": anchored pattern to ignore config at root
    const char *gitignore_contents =
        "*.log\n"
        "!important.log\n"
        "build/\n"
        "temp*\n"
        "!temp_keep.txt\n"
        "/config/\n";
    create_file(temp_dir, ".gitignore", gitignore_contents);
    
    GitignoreList gitignore;
    load_gitignore(temp_dir, &gitignore);
    
    // Test extension wildcard
    // "error.log" should be ignored, but "important.log" should not.
    assert(match_gitignore("error.log", &gitignore) == 1);
    assert(match_gitignore("important.log", &gitignore) == 0);
    
    // Test directory pattern "build/"
    // "build" and any file inside "build" should be ignored.
    assert(match_gitignore("build", &gitignore) == 1);
    assert(match_gitignore("build/main.o", &gitignore) == 1);
    
    // Test wildcard prefix "temp*"
    // "tempfile.txt" should be ignored, but "temp_keep.txt" should not.
    assert(match_gitignore("tempfile.txt", &gitignore) == 1);
    assert(match_gitignore("temp_keep.txt", &gitignore) == 0);
    
    // Test anchored pattern "/config/"
    // "config" at root should be ignored but "src/config" should not.
    assert(match_gitignore("config", &gitignore) == 1);
    assert(match_gitignore("src/config", &gitignore) == 0);
    
    free_gitignore(&gitignore);
    printf("✔ test_gitignore_wildcards passed\n");
    
#ifndef INSPECT_TEMP
    if (remove_directory_recursive(temp_dir) == 0) {
        printf("Folder '%s' removed successfully.\n", temp_dir);
    } else {
        printf("Failed to remove folder '%s'.\n", temp_dir);
    }
#endif
    free(temp_dir);
}

/* Test for a folder where all files are ignored by .gitignore.
 * This test creates a temporary directory with a .gitignore that ignores all *.txt files.
 * It then creates only .txt files in the directory.
 * We expect document_directory() to complete and warn that all files were ignored.
 */
void test_all_ignored_files() {
    char *temp_dir = create_temp_dir();
    // Create a .gitignore that ignores all .txt files.
    create_file(temp_dir, ".gitignore", "*.txt\n");
    
    // Create several .txt files.
    create_file(temp_dir, "file1.txt", "Content 1");
    create_file(temp_dir, "file2.txt", "Content 2");
    
    // Call document_directory; output file will be the default.
    const char *output_file = get_default_output(temp_dir);
    int ret = document_directory(temp_dir, output_file, 0);
    // Even though all files are ignored, the directory is non-empty so we expect success (return 0)
    assert(ret == 0);
    
    // Open the output file and check that it exists (may only have the structure header).
    FILE *f = fopen(output_file, "r");
    assert(f != NULL);
    fclose(f);
    
    printf("✔ test_all_ignored_files passed\n");
    
    // Cleanup: remove the output file and the temporary directory.
    remove(output_file);
#ifndef INSPECT_TEMP
    if (remove_directory_recursive(temp_dir) == 0) {
        printf("Folder '%s' removed successfully.\n", temp_dir);
    } else {
        printf("Failed to remove folder '%s'.\n", temp_dir);
    }
#endif
    free(temp_dir);
}

/**
 * @brief Test for the new hierarchical compare_entries() function.
 *
 * This test creates several FileEntry objects with paths that test various edge cases.
 */
void test_compare_entries() {
    // Test that a parent directory comes before its child file.
    FileEntry fe1, fe2;
    fe1.path = strdup("src");
    fe1.is_dir = true;
    fe1.depth = 0;
    fe2.path = strdup("src/main.c");
    fe2.is_dir = false;
    fe2.depth = 1;
    int cmp = compare_entries(&fe1, &fe2);
    assert(cmp < 0);  // "src" should come before "src/main.c"

    // Reverse order should yield a positive value.
    cmp = compare_entries(&fe2, &fe1);
    assert(cmp > 0);

    // Test identical paths.
    FileEntry fe3, fe4;
    fe3.path = strdup("docs/readme.md");
    fe3.is_dir = false;
    fe3.depth = 1;
    fe4.path = strdup("docs/readme.md");
    fe4.is_dir = false;
    fe4.depth = 1;
    cmp = compare_entries(&fe3, &fe4);
    assert(cmp == 0);

    // Test ordering of two distinct top-level directories.
    FileEntry fe5, fe6;
    fe5.path = strdup("a");
    fe5.is_dir = true;
    fe5.depth = 0;
    fe6.path = strdup("b");
    fe6.is_dir = true;
    fe6.depth = 0;
    cmp = compare_entries(&fe5, &fe6);
    assert(cmp < 0);  // "a" comes before "b"

    // Test ordering with multiple components.
    FileEntry fe7, fe8;
    fe7.path = strdup("a/b/c");
    fe7.is_dir = false;
    fe7.depth = 2;
    fe8.path = strdup("a/b/d");
    fe8.is_dir = false;
    fe8.depth = 2;
    cmp = compare_entries(&fe7, &fe8);
    assert(cmp < 0);  // "c" comes before "d"

    free(fe1.path);
    free(fe2.path);
    free(fe3.path);
    free(fe4.path);
    free(fe5.path);
    free(fe6.path);
    free(fe7.path);
    free(fe8.path);

    printf("✔ test_compare_entries passed\n");
}

/* Test the directory scanner:
 * Create a temporary directory structure with files and directories,
 * then scan it and verify that the file count is as expected.
 */
void test_scan_directory() {
    char *temp_dir = create_temp_dir();
    // Create a couple of files and a subdirectory
    create_file(temp_dir, "file1.txt", "Hello, world!");
    create_file(temp_dir, "file2.txt", "Test file content.");
    
    char subdir_path[MAX_PATH_LEN];
    snprintf(subdir_path, sizeof(subdir_path), "%s/subdir", temp_dir);
    if (mkdir(subdir_path, 0755) != 0) {
        perror("mkdir subdir");
        exit(EXIT_FAILURE);
    }
    create_file(subdir_path, "file3.txt", "Inside subdir.");
    
    FileList list;
    init_file_list(&list);
    // No .gitignore in effect for this test
    bool success = scan_directory(temp_dir, NULL, &list, 0, NULL, INCLUDE_GIT);
    assert(success);
    // We expect at least 4 entries: file1.txt, file2.txt, subdir, and file3.txt inside subdir.
    assert(list.count >= 4);
    free_file_list(&list);
    printf("✔ test_scan_directory passed\n");
    
#ifndef INSPECT_TEMP
    if (remove_directory_recursive(temp_dir) == 0) {
        printf("Folder '%s' removed successfully.\n", temp_dir);
    } else {
        printf("Failed to remove folder '%s'.\n", temp_dir);
    }
#endif
    free(temp_dir);
}

/* Test stats functions by counting tokens and checking backtick counts */
void test_stats() {
    const char *sample = "Hello, world!\nThis is a test.\n```\n";
    DocumentInfo info = {0};
    calculate_token_stats(sample, &info);
    assert(info.total_tokens > 0);
    int ticks = count_max_backticks("Here are ```` backticks");
    assert(ticks == 4);
    printf("✔ test_stats passed\n");
}

/* Test detecting a binary file.
 * Creates a temporary binary file and asserts that is_binary_file returns true.
 */
void test_is_binary_file() {
    char *temp_dir = create_temp_dir();
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%s", temp_dir, "binary.bin");
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    // Write 100 bytes of non-printable binary data.
    unsigned char data[100];
    memset(data, 0, sizeof(data));
    fwrite(data, 1, sizeof(data), f);
    fclose(f);
    assert(is_binary_file(path) == true);
    printf("✔ test_is_binary_file passed\n");

    // Also test that a known text file is not considered binary.
    snprintf(path, sizeof(path), "%s/%s", temp_dir, "text.txt");
    f = fopen(path, "w");
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fputs("This is a simple text file.", f);
    fclose(f);
    assert(is_binary_file(path) == false);
    printf("✔ test_is_text_file detection passed\n");
    test_smart_split();

#ifndef INSPECT_TEMP
    remove(path);
    if (remove_directory_recursive(temp_dir) == 0) {
        printf("Folder '%s' removed successfully.\n", temp_dir);
    } else {
        printf("Failed to remove folder '%s'.\n", temp_dir);
    }
#endif
    free(temp_dir);
}

/* Test the extra ignore pattern functionality with the -ngi option.
 * This test creates a temporary directory with a .gitignore that would normally ignore file2.txt.
 * With the -ngi flag, the .gitignore file is ignored, and an extra ignore pattern is provided
 * to ignore file1.txt. The output should include file2.txt but not file1.txt.
 */
 void test_ignore_extra_patterns_with_ngi() {
    char *temp_dir = create_temp_dir();
    // Create a .gitignore that ignores file2.txt (will be ignored because of -ngi)
    create_file(temp_dir, ".gitignore", "file2.txt\n");
    // Create two files.
    create_file(temp_dir, "file1.txt", "Content of file 1");
    create_file(temp_dir, "file2.txt", "Content of file 2");

    char output_file[MAX_PATH_LEN];
    snprintf(output_file, sizeof(output_file), "%s/%s", temp_dir, "test_ignore.md");

    // Set the IGNORE_GITIGNORE flag (-ngi) so .gitignore is not used.
    int flags = IGNORE_GITIGNORE;
    // Provide an extra ignore pattern to ignore file1.txt.
    char *patterns[1] = {"file1.txt"};
    set_extra_ignore_patterns(patterns, 1);

    int ret = document_directory(temp_dir, output_file, flags);
    // Clean up the extra ignore patterns before proceeding
    free_extra_ignore_patterns();

    FILE *f = fopen(output_file, "r");
    assert(f != NULL);
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *content = malloc(fsize + 1);
    fread(content, 1, fsize, f);
    content[fsize] = '\0';
    fclose(f);

    // Ensure file1.txt is not mentioned, and file2.txt is mentioned.
    assert(strstr(content, "file1.txt") == NULL);
    assert(strstr(content, "file2.txt") != NULL);
    free(content);

    remove(output_file);
#ifndef INSPECT_TEMP
    if (remove_directory_recursive(temp_dir) == 0) {
        printf("Folder '%s' removed successfully.\n", temp_dir);
    } else {
        printf("Failed to remove folder '%s'.\n", temp_dir);
    }
#endif
    free(temp_dir);
    printf("✔ test_ignore_extra_patterns_with_ngi passed\n");
}

/* Main test runner */
int main(void) {
    printf("Running tests for dirdoc...\n");
    test_get_default_output();
    test_get_language_from_extension();
    test_gitignore();
    test_gitignore_wildcards();
    test_all_ignored_files();
    test_compare_entries();
    test_scan_directory();
    test_stats();
    test_is_binary_file();
    test_ignore_extra_patterns_with_ngi();
    printf("✅ All tests passed!\n");

    // Attempt to remove the local "tmp" folder if it is empty.
#ifndef INSPECT_TEMP
    int r = remove_empty_directory("tmp");
    if (r == 0) {
        printf("Empty folder 'tmp' removed successfully.\n");
    } else if (r == 1) {
        printf("Folder 'tmp' is not empty; not removed.\n");
    } else {
        printf("Failed to remove folder 'tmp'.\n");
    }
#endif
    return 0;
}
