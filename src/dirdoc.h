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
#define SPLIT_OUTPUT     0x04  // New flag: split output into multiple files
#define INCLUDE_GIT      0x08  // New flag: include .git folders

typedef struct {
    char *path;
    bool is_dir;
    int depth;
} FileEntry;

typedef struct {
    size_t total_size;
    size_t total_tokens;
} DocumentInfo;

/* Main documentation generation function.
 * Returns 0 on success, non-zero on failure.
 */
int document_directory(const char *input_dir, const char *output_file, int flags);

/* Returns a default output filename based on the input directory.
 * The returned string is dynamically allocated and must be freed by the caller.
 */
char *get_default_output(const char *input_dir);

#ifdef __cplusplus
}
#endif

#endif /* DIRDOC_H */
