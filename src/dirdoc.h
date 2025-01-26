// dirdoc.h
#ifndef DIRDOC_H
#define DIRDOC_H

#include "cosmopolitan.h"

#define FENCE "```````"
#define MAX_PATH_LEN 4096
#define BUFFER_SIZE 4096

typedef struct {
    char *path;
    bool is_dir;
    int depth;
} FileEntry;

typedef struct {
    size_t char_count;
    size_t word_count;
    size_t token_count;
} DocumentStats;

int document_directory(const char *input_dir, const char *output_file);
bool is_binary_file(const char *path);
char *get_file_hash(const char *path);
char *get_file_size(const char *path);
void write_file_content(FILE *out, const char *path, DocumentStats *stats);
void calculate_token_stats(const char *str, DocumentStats *stats);

#endif