#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "stats.h"
#include "tiktoken.h"

// Global tiktoken encoder instance
static tiktoken_t encoder = NULL;

/**
 * @brief Initializes the tiktoken encoder
 * 
 * @return true if initialization succeeded
 * @return false if initialization failed
 */
bool init_tiktoken() {
    if (encoder == NULL) {
        // Initialize the tiktoken library first
        if (!tiktoken_init()) {
            return false;
        }
        // Initialize with cl100k_base encoding (GPT-4/3.5-turbo encoding)
        encoder = tiktoken_get_encoding("cl100k_base");
        return (encoder != NULL);
    }
    return true;
}

/**
 * @brief Frees the tiktoken encoder resources
 */
void cleanup_tiktoken() {
    if (encoder != NULL) {
        tiktoken_free(encoder);
        encoder = NULL;
    }
}

/**
 * @brief Calculates token and size statistics for the given string.
 *
 * Uses tiktoken to count tokens the same way LLMs like GPT-3.5/GPT-4 do,
 * and accumulates the total size in bytes.
 *
 * @param str The input string.
 * @param info Pointer to the DocumentInfo structure to update.
 */
void calculate_token_stats(const char *str, DocumentInfo *info) {
    size_t len = strlen(str);
    info->total_size += len;
    
    // Ensure the encoder is initialized
    if (!init_tiktoken()) {
        // Fallback to approximate calculation if tiktoken fails
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
        return;
    }
    
    // Tokenize the string using tiktoken
    tiktoken_token_t* tokens = NULL;
    int count = tiktoken_encode(encoder, str, len, &tokens);
    
    // Update token count
    if (count >= 0) {
        info->total_tokens += (size_t)count;
    } else {
        // Fallback if encoding failed
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
    
    // Free memory
    if (tokens != NULL) {
        free(tokens);
    }
}

/**
 * @brief Counts the maximum number of consecutive backticks in the given content.
 *
 * Useful for determining the number of backticks needed for fenced code blocks.
 *
 * @param content The input content string.
 * @return int The maximum consecutive count of backticks.
 */
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

/**
 * @brief Determines the programming language based on a file's extension.
 *
 * Returns a language string for syntax highlighting in Markdown fenced code blocks.
 *
 * @param filename The name or path of the file.
 * @return const char* A string representing the programming language, or an empty string if not identified.
 */
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

/**
 * @brief Returns a human-readable file size for the given file path.
 *
 * Converts the file size into a more understandable unit (B, KB, MB, etc.).
 *
 * @param path The file path.
 * @return char* A string representing the file size, or "unknown" if the file cannot be accessed.
 */
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

/**
 * @brief Determines if a file is binary by checking its printable character ratio.
 *
 * Reads up to 1024 bytes and calculates the ratio of printable characters.
 * If the ratio is below 85%, the file is considered binary.
 *
 * @param path The file path.
 * @return true If the file is binary.
 * @return false Otherwise.
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

/**
 * @brief Determines if a file should be treated as a text file based on its extension.
 *
 * Returns false for common binary file extensions.
 *
 * @param filename The name or path of the file.
 * @return true If the file is likely a text file.
 * @return false If the file is likely binary.
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
