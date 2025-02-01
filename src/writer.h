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
 * @param out_path: Path of the output file.
 * @param info: Pointer to DocumentInfo containing the computed statistics.
 */
int finalize_output(const char *out_path, DocumentInfo *info);

#endif // WRITER_H
