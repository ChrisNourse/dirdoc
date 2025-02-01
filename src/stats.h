#ifndef STATS_H
#define STATS_H

#include "dirdoc.h"

/* Calculates token and size statistics for the given string and updates DocumentInfo.
 * @param str: Input string.
 * @param info: Pointer to DocumentInfo to update.
 */
void calculate_token_stats(const char *str, DocumentInfo *info);

/* Counts the maximum consecutive backticks in the given content.
 * @param content: The string to analyze.
 * @return: The maximum count of consecutive backticks.
 */
int count_max_backticks(const char *content);

/* Determines the programming language based on the file extension.
 * @param filename: The name or path of the file.
 * @return: A string representing the language (or empty string if not identified).
 */
const char *get_language_from_extension(const char *filename);

/* Returns a human-readable file size for the file at the given path.
 * @param path: The file path.
 * @return: A string representing the file size.
 */
char *get_file_size(const char *path);

/* Checks if the file at the given path is binary.
 * @param path: The file path.
 * @return: true if binary, false otherwise.
 */
bool is_binary_file(const char *path);

#endif // STATS_H
