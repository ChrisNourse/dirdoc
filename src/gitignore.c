#include "gitignore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fnmatch.h>
#include <sys/stat.h>

#define MAX_PATH_LEN 4096

/**
 * @brief Parses a single .gitignore pattern line.
 *
 * Adjusts the pattern for negation, anchoring, and directory-only options.
 *
 * @param line The line containing the pattern.
 * @param is_negation Pointer to a boolean that is set to true if the pattern is negated.
 * @param is_anchored Pointer to a boolean that is set to true if the pattern is anchored.
 * @param is_dir_only Pointer to a boolean that is set to true if the pattern applies only to directories.
 */
static void parse_pattern(char *line, bool *is_negation, bool *is_anchored, bool *is_dir_only) {
    if (line[0] == '!') {
        *is_negation = true;
        memmove(line, line + 1, strlen(line));
    } else {
        *is_negation = false;
    }
    if (line[0] == '/') {
        *is_anchored = true;
        memmove(line, line + 1, strlen(line));
    } else {
        *is_anchored = false;
    }
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '/') {
        *is_dir_only = true;
        line[len - 1] = '\0';
    } else {
        *is_dir_only = false;
    }
}

/**
 * @brief Loads the .gitignore file from the given directory.
 *
 * Reads each non-comment, non-empty line from the .gitignore file and stores the pattern along with
 * its negation, anchoring, and directory-only properties.
 *
 * @param dir_path The directory path.
 * @param gitignore Pointer to a GitignoreList structure to populate.
 */
void load_gitignore(const char *dir_path, GitignoreList *gitignore) {
    gitignore->patterns = NULL;
    gitignore->negation = NULL;
    gitignore->dir_only = NULL;
    gitignore->anchored = NULL;
    gitignore->count    = 0;

    char gitignore_path[MAX_PATH_LEN];
    snprintf(gitignore_path, sizeof(gitignore_path), "%s/.gitignore", dir_path);

    FILE *f = fopen(gitignore_path, "r");
    if (!f) return;

    size_t capacity = 128;
    gitignore->patterns = malloc(sizeof(char*) * capacity);
    gitignore->negation = malloc(sizeof(bool)  * capacity);
    gitignore->dir_only = malloc(sizeof(bool)  * capacity);
    gitignore->anchored = malloc(sizeof(bool)  * capacity);

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || isspace((unsigned char)line[len - 1]))) {
            line[--len] = '\0';
        }
        if (len == 0 || line[0] == '#') continue;

        if (gitignore->count >= capacity) {
            capacity *= 2;
            gitignore->patterns = realloc(gitignore->patterns, sizeof(char*) * capacity);
            gitignore->negation = realloc(gitignore->negation, sizeof(bool)  * capacity);
            gitignore->dir_only = realloc(gitignore->dir_only, sizeof(bool)  * capacity);
            gitignore->anchored = realloc(gitignore->anchored, sizeof(bool)  * capacity);
        }

        char *pattern = strdup(line);
        bool neg = false, anc = false, dir_only = false;
        parse_pattern(pattern, &neg, &anc, &dir_only);

        gitignore->patterns[gitignore->count] = pattern;
        gitignore->negation[gitignore->count] = neg;
        gitignore->anchored[gitignore->count] = anc;
        gitignore->dir_only[gitignore->count] = dir_only;
        gitignore->count++;
    }
    fclose(f);
}

/**
 * @brief Checks if the parent folder matches a given pattern.
 *
 * This is used to handle cases where patterns like "folder/*" are intended to ignore the folder itself.
 *
 * @param path The file or directory path.
 * @param pattern The pattern to compare.
 * @return true if the parent folder matches the pattern, false otherwise.
 */
static bool parent_folder_match(const char *path, const char *pattern) {
    // If pattern ends with "/*", skip the folder itself
    // e.g. "builds/*" also ignores "builds"
    size_t plen = strlen(pattern);
    if (plen >= 2 && strcmp(pattern + plen - 2, "/*") == 0) {
        // Extract the folder prefix from pattern (everything before "/*")
        char *folder_prefix = strndup(pattern, plen - 2);
        // If path matches exactly that folder prefix, consider it a match
        bool is_match = (strcmp(path, folder_prefix) == 0);
        free(folder_prefix);
        return is_match;
    }
    return false;
}

/**
 * @brief Determines whether the given path should be ignored based on the .gitignore patterns.
 *
 * Checks the file or directory against each pattern in the GitignoreList and applies negations and directory-only logic.
 *
 * @param path The file or directory path.
 * @param gitignore Pointer to the GitignoreList structure.
 * @return true if the path should be ignored, false otherwise.
 */
bool match_gitignore(const char *path, const GitignoreList *gitignore) {
    if (!gitignore || !gitignore->patterns) {
        return false;
    }

    struct stat st;
    bool is_dir = false;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        is_dir = true;
    }

    bool ignored = false;
    for (size_t i = 0; i < gitignore->count; i++) {
        const char *pattern = gitignore->patterns[i];
        bool neg   = gitignore->negation[i];
        bool anc   = gitignore->anchored[i];
        bool donly = gitignore->dir_only[i];

        if (donly) {
            /* 
             * For directory-only patterns, match if the path is exactly the pattern
             * or if it starts with pattern followed by a '/'.
             */
            size_t pat_len = strlen(pattern);
            if (strncmp(path, pattern, pat_len) == 0 && (path[pat_len] == '\0' || path[pat_len] == '/')) {
                ignored = !neg;
            }
            continue;
        }

        // Handle the "folder/*" case => ignore the folder itself
        if (parent_folder_match(path, pattern)) {
            ignored = !neg;
            continue;
        }

        int flags = FNM_PATHNAME;
        if (anc) {
            // If anchored, skip if path has subdirs, for simplicity
            // (real .gitignore anchoring is more nuanced)
            if (strchr(path, '/')) {
                continue;
            }
        }

        if (fnmatch(pattern, path, flags) == 0) {
            ignored = !neg;
        }
    }
    return ignored;
}

/**
 * @brief Frees the memory allocated for the GitignoreList.
 *
 * Releases all patterns and associated arrays, then resets the structure.
 *
 * @param gitignore Pointer to the GitignoreList to free.
 */
void free_gitignore(GitignoreList *gitignore) {
    if (!gitignore || !gitignore->patterns) {
        return;
    }
    for (size_t i = 0; i < gitignore->count; i++) {
        free(gitignore->patterns[i]);
    }
    free(gitignore->patterns);
    free(gitignore->negation);
    free(gitignore->dir_only);
    free(gitignore->anchored);
    gitignore->patterns = NULL;
    gitignore->negation = NULL;
    gitignore->dir_only = NULL;
    gitignore->anchored = NULL;
    gitignore->count    = 0;
}
