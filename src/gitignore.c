#include "gitignore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <sys/stat.h>

#define MAX_LINE_LENGTH 1024

// Helper: Escapes regex special characters in a string.
static char *escape_regex(const char *str) {
    size_t len = strlen(str);
    char *escaped = (char*)malloc(2 * len + 1); // worst-case: every character escaped
    if (!escaped) return NULL;
    char *dest = escaped;
    for (size_t i = 0; i < len; i++) {
        if (strchr(".^$+?()[]{}|\\", str[i])) {
            *dest++ = '\\';
        }
        *dest++ = str[i];
    }
    *dest = '\0';
    return escaped;
}

// Helper: Translates a gitignore pattern into a POSIX regular expression.
// This simplified translator replaces:
//   - "**" with ".*" (matching across directories)
//   - "*"  with "[^/]*" (matching within a single directory)
//   - "?"  with "." (any single character)
// Change the function signature to accept dir_only flag
static char *translate_gitignore_pattern(const char *pattern, bool dir_only) {
    size_t len = strlen(pattern);
    // Allocate a buffer that is large enough for worst-case expansion.
    char *regex_pattern = malloc(4 * len + 16); // Added extra space for directory pattern
    if (!regex_pattern) return NULL;
    char *dest = regex_pattern;
    
    // If the pattern is anchored (starts with '/'), then match beginning of string.
    if (pattern[0] == '/') {
        *dest++ = '^';
        pattern++; // skip leading '/'
    } else {
        // Not anchored: allow match anywhere in the string.
        *dest++ = '^';
        *dest++ = '.';
        *dest++ = '*';
    }
    
    while (*pattern) {
        if (pattern[0] == '*' && pattern[1] == '*') {
            // Handle "**": match any sequence (including '/')
            pattern += 2;
            if (*pattern == '/') {
                pattern++;
            }
            strcpy(dest, ".*");
            dest += 2;
        } else if (*pattern == '*') {
            pattern++;
            // Single '*' matches any sequence except '/'
            strcpy(dest, "[^/]*");
            dest += strlen("[^/]*");
        } else if (*pattern == '?') {
            pattern++;
            *dest++ = '.';
        } else {
            // Escape regex special characters if needed.
            if (strchr(".^$+?()[]{}|\\", *pattern)) {
                *dest++ = '\\';
            }
            *dest++ = *pattern++;
        }
    }
    
    // For directory patterns, append a pattern that matches paths inside the directory
    if (dir_only) {
        strcpy(dest, "(/.*)?$");
        dest += strlen("(/.*)?$");
    } else {
        // Regular end of pattern marker
        *dest++ = '$';
        *dest = '\0';
    }
    
    return regex_pattern;
}

// Update the parse_gitignore_line function to pass dir_only to translate_gitignore_pattern
/* Parses a single gitignore pattern string and adds it to the provided GitignoreList.
 * Returns 0 on success, -1 on error.
 */
int parse_gitignore_pattern_string(const char *pattern_str, GitignoreList *list) {
    if (!pattern_str || !list) return -1;
    
    // Skip empty patterns
    if (*pattern_str == '\0') return 0;
    
    // Duplicate the pattern so we can modify it.
    char *pattern = strdup(pattern_str);
    if (!pattern) return -1;
    
    // Parse pattern flags
    bool negation = false;
    if (pattern[0] == '!') {
        negation = true;
        memmove(pattern, pattern + 1, strlen(pattern));
    }
    
    bool anchored = false;
    if (pattern[0] == '/') {
        anchored = true;
    }
    
    bool dir_only = false;
    size_t len = strlen(pattern);
    if (len > 0 && pattern[len - 1] == '/') {
        dir_only = true;
        pattern[len - 1] = '\0'; // remove trailing '/'
    }
    
    // Generate regex pattern from gitignore pattern
    char *regex_str = translate_gitignore_pattern(pattern, dir_only);
    if (!regex_str) {
        free(pattern);
        return -1;
    }
    
    // Compile the regex
    regex_t regex;
    int ret = regcomp(&regex, regex_str, REG_EXTENDED | REG_NOSUB);
    free(regex_str);
    if (ret != 0) {
        free(pattern);
        return -1;
    }
    
    // Ensure capacity in the GitignoreList
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity ? list->capacity * 2 : 16;
        GitignoreRule *new_rules = realloc(list->rules, new_capacity * sizeof(GitignoreRule));
        if (!new_rules) {
            regfree(&regex);
            free(pattern);
            return -1;
        }
        list->rules = new_rules;
        list->capacity = new_capacity;
    }
    
    // Add the new rule
    GitignoreRule *rule = &list->rules[list->count++];
    rule->pattern = pattern;
    rule->negation = negation;
    rule->anchored = anchored;
    rule->dir_only = dir_only;
    rule->regex = regex;
    
    return 0;
}

static int parse_gitignore_line(const char *line, GitignoreList *list) {
    // Skip leading whitespace.
    while (*line && isspace((unsigned char)*line)) line++;
    if (*line == '\0' || *line == '#') return 0; // skip empty or comment lines
    
    // Trim trailing whitespace before processing.
    char *trimmed_line = strdup(line);
    if (!trimmed_line) return -1;
    
    size_t l = strlen(trimmed_line);
    while (l > 0 && isspace((unsigned char)trimmed_line[l - 1])) {
        trimmed_line[--l] = '\0';
    }
    
    // Use the common pattern parsing function
    int result = parse_gitignore_pattern_string(trimmed_line, list);
    free(trimmed_line);
    return result;
}

void load_gitignore(const char *dir_path, GitignoreList *gitignore) {
    gitignore->rules = NULL;
    gitignore->count = 0;
    gitignore->capacity = 0;
    
    char gitignore_path[4096];
    snprintf(gitignore_path, sizeof(gitignore_path), "%s/.gitignore", dir_path);
    
    FILE *f = fopen(gitignore_path, "r");
    if (!f) return;
    
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), f)) {
        // Remove newline characters.
        line[strcspn(line, "\r\n")] = '\0';
        if (parse_gitignore_line(line, gitignore) != 0) {
            // On error, skip this rule.
            continue;
        }
    }
    fclose(f);
}

bool match_gitignore(const char *path, const GitignoreList *gitignore) {
    if (!gitignore || gitignore->count == 0) return false;
    
    bool ignored = false;
    for (size_t i = 0; i < gitignore->count; i++) {
        const GitignoreRule *rule = &gitignore->rules[i];
        int ret = regexec(&rule->regex, path, 0, NULL, 0);
        if (ret == 0) {
            // If the rule matches, update the ignored state based on whether it is a negation.
            ignored = !rule->negation;
        }
    }
    return ignored;
}

void free_gitignore(GitignoreList *gitignore) {
    if (!gitignore) return;
    for (size_t i = 0; i < gitignore->count; i++) {
        GitignoreRule *rule = &gitignore->rules[i];
        regfree(&rule->regex);
        free(rule->pattern);
    }
    free(gitignore->rules);
    gitignore->rules = NULL;
    gitignore->count = 0;
    gitignore->capacity = 0;
}
