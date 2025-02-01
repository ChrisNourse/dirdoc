#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "stats.h"

void calculate_token_stats(const char *str, DocumentInfo *info) {
    bool in_word = false;
    const char *p = str;
    
    while (*p) {
        if (isspace((unsigned char)*p) || *p == '\n' || *p == '\t') {
            if (in_word) {
                info->total_tokens++;
                in_word = false;
            }
        } else {
            if (!in_word) {
                in_word = true;
            }
        }
        
        if (*p == '#' || *p == '*' || *p == '_' || *p == '`' || *p == '[' ||
            *p == ']' || *p == '(' || *p == ')' || *p == '|' || *p == '-') {
            info->total_tokens++;
        }
        
        info->total_size++;
        p++;
    }
    if (in_word) {
        info->total_tokens++;
    }
}

int count_max_backticks(const char *content) {
    int max_count = 0;
    int current_count = 0;
    const char *p = content;
    
    while (*p) {
        if (*p == '`') {
            current_count++;
        } else {
            if (current_count > max_count) {
                max_count = current_count;
            }
            current_count = 0;
        }
        p++;
    }
    if (current_count > max_count) {
        max_count = current_count;
    }
    return max_count;
}

const char *get_language_from_extension(const char *filename) {
    const char *base = strrchr(filename, '/');
    base = base ? base + 1 : filename;
    
    // Check for common makefile names (case-insensitive)
    if (strcasecmp(base, "Makefile") == 0 || 
        strcasecmp(base, "GNUmakefile") == 0)
    {
        return "make";
    }

    // Find the last dot in the base filename
    const char *dot = strrchr(base, '.');
    if (!dot || dot == base) return "";
    dot++; // Move past the dot

    // Compare the extension (case-insensitive) to known types
    if (strcasecmp(dot, "c") == 0) return "c";
    if (strcasecmp(dot, "h") == 0) return "c";
    if (strcasecmp(dot, "cpp") == 0) return "cpp";
    if (strcasecmp(dot, "cc") == 0) return "cpp";
    if (strcasecmp(dot, "hpp") == 0) return "cpp";
    if (strcasecmp(dot, "md") == 0) return "markdown";
    if (strcasecmp(dot, "sql") == 0) return "sql";
    if (strcasecmp(dot, "sh") == 0) return "bash";
    if (strcasecmp(dot, "py") == 0) return "python";
    if (strcasecmp(dot, "js") == 0) return "javascript";
    if (strcasecmp(dot, "json") == 0) return "json";
    if (strcasecmp(dot, "html") == 0) return "html";

    return "";
}

char *get_file_size(const char *path) {
    static char size[32];
    struct stat st;
    if (stat(path, &st) != 0) {
        return "unknown";
    }
    double size_bytes = st.st_size;
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    
    while (size_bytes >= 1024 && unit < 4) {
        size_bytes /= 1024;
        unit++;
    }
    
    snprintf(size, sizeof(size), "%.2f %s", size_bytes, units[unit]);
    return size;
}

bool is_binary_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return true;
    
    unsigned char buf[1024];
    size_t len = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == 0) return true;
    }
    return false;
}
