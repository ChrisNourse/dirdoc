#ifndef DIRDOC_H
#define DIRDOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#define MAX_PATH_LEN 4096
#define BUFFER_SIZE 4096

// Flag bits
#define IGNORE_GITIGNORE 0x01
#define STRUCTURE_ONLY   0x02

typedef struct {
    char *path;
    bool is_dir;
    int depth;
} FileEntry;

typedef struct {
    size_t total_size;
    size_t total_tokens;
} DocumentInfo;

/* Generates documentation for the specified directory.
 * Returns 0 on success, non-zero on failure.
 */
int document_directory(const char *input_dir, const char *output_file, int flags);

char *get_default_output(const char *input_dir);
bool is_binary_file(const char *path);
char *get_file_hash(const char *path);
char *get_file_size(const char *path);
void write_file_content(FILE *out, const char *path, DocumentInfo *info);
/* Updates token and size counters in DocumentInfo based on the provided string. */
void calculate_token_stats(const char *str, DocumentInfo *info);
int count_max_backticks(const char *content);
void print_terminal_stats(const char *output_path, const DocumentInfo *info);


/* Returns a default output filename based on the input directory. */
char *get_default_output(const char *input_dir);

#ifdef __cplusplus
}
#endif

#endif /* DIRDOC_H */
