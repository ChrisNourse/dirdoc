#ifndef GITIGNORE_H
#define GITIGNORE_H

#include <stdbool.h>
#include <stddef.h>
#include <regex.h>

// A single gitignore rule, including its original pattern and compiled regex.
typedef struct {
    char *pattern;      // original gitignore pattern (e.g. "**/*.log")
    bool negation;      // true if the rule is a negation (starts with '!')
    bool anchored;      // true if the pattern is anchored (starts with '/')
    bool dir_only;      // true if the rule applies only to directories (ends with '/')
    regex_t regex;      // compiled regular expression for matching paths
} GitignoreRule;

// A collection of gitignore rules.
typedef struct {
    GitignoreRule *rules;
    size_t count;
    size_t capacity;
} GitignoreList;

/**
 * @brief Load gitignore rules from a directory.
 *
 * @param dir_path Directory containing a .gitignore file.
 * @param gitignore Gitignore list to populate.
 */
void load_gitignore(const char *dir_path, GitignoreList *gitignore);

/**
 * @brief Add a gitignore pattern string to a list.
 *
 * @param pattern Pattern text.
 * @param list Gitignore list to modify.
 * @return int 0 on success, -1 on error.
 */
int parse_gitignore_pattern_string(const char *pattern, GitignoreList *list);

/**
 * @brief Check if a path matches gitignore rules.
 *
 * @param path Relative path to test.
 * @param gitignore Compiled gitignore list.
 * @return true if the path should be ignored.
 */
bool match_gitignore(const char *path, const GitignoreList *gitignore);

/**
 * @brief Free resources associated with a GitignoreList.
 */
void free_gitignore(GitignoreList *gitignore);

#endif
