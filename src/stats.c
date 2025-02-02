#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "stats.h"

void calculate_token_stats(const char *str, DocumentInfo *info) {
    size_t len = strlen(str);
    info->total_size += len;
    size_t i = 0;
    while (i < len) {
        if (isspace((unsigned char)str[i])) {
            i++;
            continue;
        }
        if (isalnum((unsigned char)str[i]) || str[i] == '_') {
            info->total_tokens++;
            while (i < len && (isalnum((unsigned char)str[i]) || str[i] == '_')) {
                i++;
            }
        } else {
            info->total_tokens++;
            i++;
        }
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

/* 
 * Improved binary file detection:
 * Reads up to 1024 bytes and calculates the ratio of printable characters.
 * Allowed printable characters are: tab (9), line feed (10), carriage return (13),
 * and characters in the range 32 to 126 inclusive.
 * If the ratio of printable characters is below 85%, the file is considered binary.
 */
bool is_binary_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return true;
    
    unsigned char buf[1024];
    size_t len = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    
    if (len == 0) return false; // Empty file considered text
    
    size_t printable = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (c == 9 || c == 10 || c == 13 || (c >= 32 && c <= 126)) {
            printable++;
        }
    }
    double ratio = (double)printable / len;
    if (ratio < 0.85) {
        return true;
    }
    return false;
}

/*
 * Determines whether a file should be treated as a text file based on its extension.
 * Returns false for common binary file extensions.
 */
bool is_text_file_by_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return true;
    dot++; // Move past the dot
    
    char ext[16];
    snprintf(ext, sizeof(ext), "%s", dot);
    for (char *p = ext; *p; ++p) {
        *p = tolower(*p);
    }
    
    if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0 ||
        strcmp(ext, "png") == 0 || strcmp(ext, "gif") == 0 ||
        strcmp(ext, "bmp") == 0 || strcmp(ext, "tiff") == 0 ||
        strcmp(ext, "ico") == 0) {
        return false;
    }
    return true;
}
