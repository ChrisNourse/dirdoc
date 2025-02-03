#ifndef WRITER_H
#define WRITER_H

#include <stdio.h>
#include "dirdoc.h"
#include "scanner.h"
#include "stats.h"
#include "gitignore.h"

/* Writes the tree structure representation to the given file stream.
 * @param out: Output file stream.
 * @param list: Pointer to the FileList containing scanned entries.
 * @param info: Pointer to DocumentInfo for accumulating token and size statistics.
 */
void write_tree_structure(FILE *out, FileList *list, DocumentInfo *info);

/* Writes the content of a file with fenced code blocks to the output stream.
 * @param out: Output file stream.
 * @param path: Path of the file to write.
 * @param info: Pointer to DocumentInfo for accumulating token and size statistics.
 */
void write_file_content(FILE *out, const char *path, DocumentInfo *info);

/* Finalizes the output file by prepending token statistics.
 * If split output is enabled and the file exceeds the specified limit,
 * the output will be split into multiple files with increasing number suffixes.
 * @param out_path: Path of the output file.
 * @param info: Pointer to DocumentInfo containing the computed statistics.
 * @return: 0 on success, non-zero on failure.
 */
int finalize_output(const char *out_path, DocumentInfo *info);

/* Sets the split output options.
 * @param enabled: Non-zero to enable splitting.
 * @param limit_mb: Maximum size in MB per output file.
 */
void set_split_options(int enabled, double limit_mb);

/* Sets extra ignore patterns to be applied during directory scanning.
 * @param patterns: Array of pattern strings.
 * @param count: Number of patterns.
 */
void set_extra_ignore_patterns(char **patterns, int count);

/* Main documentation generation function.
 * Returns 0 on success, non-zero on failure.
 */
int document_directory(const char *input_dir, const char *output_file, int flags);

#endif // WRITER_H
