#include "gitignore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fnmatch.h>
#include <sys/stat.h>

#define MAX_PATH_LEN 4096

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

        if (donly && !is_dir) {
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
