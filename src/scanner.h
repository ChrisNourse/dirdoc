#ifndef SCANNER_H
#define SCANNER_H

#include "dirdoc.h"
#include "gitignore.h"

// Data structure for accumulating file entries during directory scanning.
typedef struct {
    FileEntry *entries;
    size_t count;
    size_t capacity;
} FileList;

/**
 * @brief Initialize a FileList structure.
 */
void init_file_list(FileList *list);

/**
 * @brief Add a new file entry to a FileList.
 *
 * @param list List to update.
 * @param path Relative path of the file or directory.
 * @param is_dir Non-zero if the entry is a directory.
 * @param depth Depth level within the tree.
 */
void add_file_entry(FileList *list, const char *path, bool is_dir, int depth);

/**
 * @brief Free memory used by a FileList.
 */
void free_file_list(FileList *list);

/**
 * @brief Recursively scan a directory and populate a FileList.
 *
 * @param dir_path Absolute path to scan.
 * @param rel_path Relative path from the root directory.
 * @param list Output FileList.
 * @param depth Current recursion depth.
 * @param gitignore Optional gitignore rule list.
 * @param flags Flags controlling scanning behavior.
 * @return true if entries were found, otherwise false.
 */
bool scan_directory(const char *dir_path, const char *rel_path, FileList *list, int depth, const GitignoreList *gitignore, int flags);

/**
 * @brief Comparison function for ordering FileEntry items.
 *
 * @return Negative if a < b, zero if equal, positive if a > b.
 */
int compare_entries(const void *a, const void *b);

#endif // SCANNER_H
