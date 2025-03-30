#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#include "scanner.h"
#include "gitignore.h"
#include "dirdoc.h"

/**
 * @brief Initializes a FileList structure.
 *
 * Allocates initial memory for file entries and sets count and capacity.
 *
 * @param list Pointer to the FileList structure.
 */
void init_file_list(FileList *list) {
    list->capacity = 16;
    list->count = 0;
    list->entries = (FileEntry*)malloc(list->capacity * sizeof(FileEntry));
}

/**
 * @brief Adds a new file entry to the FileList.
 *
 * Expands the array if needed and adds the entry with its path, directory flag, and depth.
 *
 * @param list Pointer to the FileList.
 * @param path Relative path of the file or directory.
 * @param is_dir Boolean indicating if the entry is a directory.
 * @param depth Depth level in the directory hierarchy.
 */
void add_file_entry(FileList *list, const char *path, bool is_dir, int depth) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->entries = realloc(list->entries, list->capacity * sizeof(FileEntry));
    }
    
    FileEntry *entry = &list->entries[list->count++];
    entry->path = strdup(path);
    entry->is_dir = is_dir;
    entry->depth = depth;
}

/**
 * @brief Frees all memory associated with the FileList.
 *
 * Releases the memory for each file path and the entries array.
 *
 * @param list Pointer to the FileList.
 */
void free_file_list(FileList *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->entries[i].path);
    }
    free(list->entries);
}
/**
 * @brief Compares two directory paths hierarchically.
 *
 * This function splits each input path into its individual components (using '/' as the delimiter)
 * and compares corresponding components one by one. This hierarchical comparison ensures that a
 * parent directory (e.g., "src") will always sort before any child file or directory (e.g., "src/main.c").
 *
 * @param path1 The first path string to compare.
 * @param path2 The second path string to compare.
 * @return int A negative value if path1 < path2, zero if path1 == path2, or a positive value if path1 > path2.
 */
static int compare_paths(const char *path1, const char *path2) {
    const char *p1 = path1, *p2 = path2;
    while (*p1 || *p2) {
        char comp1[256] = {0};
        char comp2[256] = {0};
        int i = 0;
        // Extract next component from path1 (up to '/' or end)
        while (*p1 && *p1 != '/') {
            comp1[i++] = *p1;
            p1++;
        }
        comp1[i] = '\0';
        if (*p1 == '/') p1++;  // Skip the '/' if present

        i = 0;
        // Extract next component from path2 (up to '/' or end)
        while (*p2 && *p2 != '/') {
            comp2[i++] = *p2;
            p2++;
        }
        comp2[i] = '\0';
        if (*p2 == '/') p2++;  // Skip the '/' if present

        // Compare the two components; if they differ, return the comparison result.
        int cmp = strcmp(comp1, comp2);
        if (cmp != 0) {
            return cmp;
        }
    }
    return 0;
}

/**
 * @brief Comparator function for sorting FileEntry items in a hierarchical order.
 *
 * This function is designed to be used with the standard qsort() function to sort a flat array of
 * FileEntry structures. It compares the `path` members of two FileEntry items using a hierarchical
 * comparison (via compare_paths()). This ensures that parent directories are grouped immediately before
 * their children in the final sorted order.
 *
 * @param a Pointer to the first FileEntry (as a void pointer).
 * @param b Pointer to the second FileEntry (as a void pointer).
 * @return int A negative value if the first entry should come before the second,
 *             zero if they are considered equal,
 *             or a positive value if the first entry should come after the second.
 */
int compare_entries(const void *a, const void *b) {
    const FileEntry *fa = (const FileEntry *)a;
    const FileEntry *fb = (const FileEntry *)b;
    return compare_paths(fa->path, fb->path);
}

/**
 * @brief Recursively scans a directory and populates the FileList with file and subdirectory entries.
 *
 * Optionally uses the provided GitignoreList to skip ignored files/directories and respects the flags.
 *
 * @param dir_path The absolute path of the directory to scan.
 * @param rel_path The relative path from the root directory (can be NULL).
 * @param list Pointer to the FileList to populate.
 * @param depth Current depth level.
 * @param gitignore Pointer to a GitignoreList (can be NULL).
 * @param flags Flags controlling scanning behavior (e.g., INCLUDE_GIT).
 * @return true if scanning was successful and entries were found, false otherwise.
 */
bool scan_directory(const char *dir_path, const char *rel_path, FileList *list, int depth, const GitignoreList *gitignore, int flags) {
    if (gitignore && rel_path && match_gitignore(rel_path, gitignore)) {
        return false;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: Directory '%s' does not exist or cannot be opened\n", dir_path);
        return false;
    }

    bool has_entries = false;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // By default, ignore .git folders unless INCLUDE_GIT flag is set.
        if (!(flags & INCLUDE_GIT) && strcmp(entry->d_name, ".git") == 0) {
            continue;
        }

        char full_path[MAX_PATH_LEN];
        char rel_entry_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        snprintf(rel_entry_path, sizeof(rel_entry_path), "%s%s%s",
                 rel_path ? rel_path : "",
                 rel_path ? "/" : "",
                 entry->d_name);

        if (gitignore && match_gitignore(rel_entry_path, gitignore)) {
            continue;
        }

        struct stat st;
        if (stat(full_path, &st) == 0) {
            bool is_subdir = S_ISDIR(st.st_mode);
            add_file_entry(list, rel_entry_path, is_subdir, depth);
            if (is_subdir) {
                has_entries |= scan_directory(full_path, rel_entry_path, list, depth + 1, gitignore, flags);
            }
        }
    }
    closedir(dir);

    return has_entries || (list->count > 0);
}
