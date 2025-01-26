#ifndef DIRDOC_H
#define DIRDOC_H

#include "cosmopolitan.h"

#define MAX_PATH_LEN 4096
#define BUFFER_SIZE 4096
#define IGNORE_GITIGNORE 0x01

typedef struct {
    char *path;
    bool is_dir;
    int depth;
} FileEntry;

typedef struct {
    size_t total_size;
    size_t total_tokens;
} DocumentInfo;

int document_directory(const char *input_dir, const char *output_file, int flags);
bool is_binary_file(const char *path);
char *get_file_hash(const char *path);
char *get_file_size(const char *path);
void write_file_content(FILE *out, const char *path, DocumentInfo *info);
void calculate_token_stats(const char *str, DocumentInfo *info);
int count_max_backticks(const char *content);
void print_terminal_stats(const char *output_path, const DocumentInfo *info);

#endif