#ifndef GITIGNORE_H
#define GITIGNORE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char **patterns;
    bool *negation;
    bool *dir_only;
    bool *anchored;
    size_t count;
} GitignoreList;

void load_gitignore(const char *dir_path, GitignoreList *gitignore);
void free_gitignore(GitignoreList *gitignore);
bool match_gitignore(const char *path, const GitignoreList *gitignore);

#endif