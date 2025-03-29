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

/* Loads the .gitignore file from the specified directory,
 * parses each rule (supporting ** patterns, negation, etc.), and compiles a regex for each.
 */
void load_gitignore(const char *dir_path, GitignoreList *gitignore);

/* Parses a single gitignore pattern string and adds it to the provided GitignoreList.
 * Returns 0 on success, -1 on error.
 */
int parse_gitignore_pattern_string(const char *pattern, GitignoreList *list);

/* Checks whether the given path should be ignored based on the compiled gitignore rules.
 * Returns true if the path should be ignored.
 */
bool match_gitignore(const char *path, const GitignoreList *gitignore);

/* Frees all memory allocated for the GitignoreList and releases compiled regex resources.
 */
void free_gitignore(GitignoreList *gitignore);

#endif
