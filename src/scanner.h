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

/* Initializes the given FileList structure. */
void init_file_list(FileList *list);

/* Adds a new file entry to the FileList.
 * @param list: Pointer to the FileList.
 * @param path: Relative path of the file or directory.
 * @param is_dir: Boolean indicating if the entry is a directory.
 * @param depth: Depth level in the directory hierarchy.
 */
void add_file_entry(FileList *list, const char *path, bool is_dir, int depth);

/* Frees all memory associated with the FileList. */
void free_file_list(FileList *list);

/* Recursively scans the directory at dir_path and populates the FileList.
 * @param dir_path: The absolute path of the directory to scan.
 * @param rel_path: The relative path from the root directory (can be NULL).
 * @param list: Pointer to the FileList to populate.
 * @param depth: Current depth level.
 * @param gitignore: Pointer to a GitignoreList structure (can be NULL).
 * @param flags: Flags controlling scanning behavior (e.g., INCLUDE_GIT)
 * @return: true if scanning was successful, false otherwise.
 */
bool scan_directory(const char *dir_path, const char *rel_path, FileList *list, int depth, const GitignoreList *gitignore, int flags);

/* Comparison function for qsort to order FileEntry items.
 * @return: Negative if a < b, zero if equal, positive if a > b.
 */
int compare_entries(const void *a, const void *b);

#endif // SCANNER_H
