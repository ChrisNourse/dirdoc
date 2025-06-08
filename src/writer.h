#ifndef WRITER_H
#define WRITER_H

#include <stdio.h>
#include "dirdoc.h"
#include "scanner.h"
#include "stats.h"
#include "gitignore.h"

/**
 * @brief Write the directory tree structure to a file stream.
 *
 * @param out Output file stream.
 * @param list List of scanned file entries.
 * @param info Document statistics accumulator.
 */
void write_tree_structure(FILE *out, FileList *list, DocumentInfo *info);

/**
 * @brief Write a file's contents to the output stream using fenced code blocks.
 *
 * @param out Output file stream.
 * @param path Path of the file to write.
 * @param info Document statistics accumulator.
 */
void write_file_content(FILE *out, const char *path, DocumentInfo *info);

/**
 * @brief Finalize the output file and optionally split it by size.
 *
 * @param out_path Path of the output file.
 * @param info Document statistics.
 * @return int 0 on success, non-zero on failure.
 */
int finalize_output(const char *out_path, DocumentInfo *info);

/**
 * @brief Configure splitting of the output file.
 *
 * @param enabled Non-zero to enable splitting.
 * @param limit_mb Maximum size per file in MB.
 */
void set_split_options(int enabled, double limit_mb);

/**
 * @brief Set additional ignore patterns for directory scanning.
 *
 * @param patterns Array of pattern strings.
 * @param count Number of patterns.
 */
void set_extra_ignore_patterns(char **patterns, int count);

/**
 * @brief Free memory allocated for extra ignore patterns.
 */
void free_extra_ignore_patterns();

/**
 * @brief Generate documentation for a directory.
 *
 * @param input_dir Directory to document.
 * @param output_file Output markdown path or NULL for default.
 * @param flags Flags controlling the documentation process.
 * @return int 0 on success, non-zero on failure.
 */
int document_directory(const char *input_dir, const char *output_file, int flags);

#endif // WRITER_H
