#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#include "scanner.h"
#include "gitignore.h"
#include "dirdoc.h"

void init_file_list(FileList *list) {
    list->capacity = 16;
    list->count = 0;
    list->entries = malloc(list->capacity * sizeof(FileEntry));
}

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

void free_file_list(FileList *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->entries[i].path);
    }
    free(list->entries);
}

int compare_entries(const void *a, const void *b) {
    const FileEntry *fa = (const FileEntry *)a;
    const FileEntry *fb = (const FileEntry *)b;
    
    char *parent_a = strdup(fa->path);
    char *parent_b = strdup(fb->path);
    char *last_slash_a = strrchr(parent_a, '/');
    char *last_slash_b = strrchr(parent_b, '/');
    
    if (last_slash_a) *last_slash_a = '\0';
    if (last_slash_b) *last_slash_b = '\0';
    
    int parent_cmp = strcmp(parent_a ? parent_a : "", parent_b ? parent_b : "");
    free(parent_a);
    free(parent_b);
    if (parent_cmp != 0) {
        return parent_cmp;
    }
    
    if (fa->is_dir != fb->is_dir) {
        return (fb->is_dir - fa->is_dir);
    }
    
    return strcmp(fa->path, fb->path);
}

bool scan_directory(const char *dir_path, const char *rel_path, FileList *list, int depth, const GitignoreList *gitignore) {
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
                has_entries |= scan_directory(full_path, rel_entry_path, list, depth + 1, gitignore);
            }
        }
    }
    closedir(dir);

    return has_entries || (list->count > 0);
}
