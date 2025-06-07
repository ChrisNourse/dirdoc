#ifndef RECONSTRUCT_H
#define RECONSTRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Reconstructs a directory from a dirdoc generated markdown file.
 * md_path: path to the documentation markdown
 * out_dir: directory to create reconstructed files
 * Returns 0 on success, non-zero on failure.
 */
int reconstruct_from_markdown(const char *md_path, const char *out_dir);

#ifdef __cplusplus
}
#endif

#endif /* RECONSTRUCT_H */
