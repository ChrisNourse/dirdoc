#ifndef RECONSTRUCT_H
#define RECONSTRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reconstruct a directory from a dirdoc markdown file.
 *
 * @param md_path Path to the documentation markdown.
 * @param out_dir Output directory to write reconstructed files.
 * @return int 0 on success, non-zero on failure.
 */
int reconstruct_from_markdown(const char *md_path, const char *out_dir);

#ifdef __cplusplus
}
#endif

#endif /* RECONSTRUCT_H */
