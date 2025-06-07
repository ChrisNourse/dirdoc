#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "reconstruct.h"
#include "dirdoc.h" // for MAX_PATH_LEN and BUFFER_SIZE

static int mkdirs(const char *path) {
    char tmp[MAX_PATH_LEN];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (len == 0)
        return 0;
    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755); // final component
}

static int is_fence_start(const char *line, int *len) {
    int i = 0;
    while (line[i] == '`') i++;
    if (i >= 3) {
        *len = i;
        return 1;
    }
    return 0;
}

static int is_fence_end(const char *line, int len) {
    for (int i = 0; i < len; i++) {
        if (line[i] != '`') return 0;
    }
    char c = line[len];
    return c == '\n' || c == '\0' || c == '\r';
}

int reconstruct_from_markdown(const char *md_path, const char *out_dir) {
    FILE *in = fopen(md_path, "r");
    if (!in) {
        fprintf(stderr, "Error: cannot open %s\n", md_path);
        return 1;
    }

    char line[BUFFER_SIZE];
    char file_path[MAX_PATH_LEN];
    FILE *out = NULL;
    int in_code = 0;
    int fence_len = 0;
    int skip_file = 0;

    while (fgets(line, sizeof(line), in)) {
        if (!in_code && strncmp(line, "### 📄 ", 8) == 0) {
            if (out) {
                fclose(out);
                out = NULL;
            }
            skip_file = 0;
            line[strcspn(line, "\r\n")] = '\0';
            snprintf(file_path, sizeof(file_path), "%s/%s", out_dir, line + 9);
            char dir[MAX_PATH_LEN];
            snprintf(dir, sizeof(dir), "%s", file_path);
            char *p = strrchr(dir, '/');
            if (p) {
                *p = '\0';
                mkdirs(dir);
            } else {
                mkdirs(out_dir);
            }
            out = fopen(file_path, "w");
            continue;
        }

        if (!in_code) {
            if (is_fence_start(line, &fence_len)) {
                in_code = 1;
                if (out) ftruncate(fileno(out), 0); // ensure empty before writing
                continue;
            }
        } else {
            if (is_fence_end(line, fence_len)) {
                in_code = 0;
                if (out) {
                    fclose(out);
                    out = NULL;
                }
                continue;
            }
            if (out && !skip_file) {
                if (strncmp(line, "*Binary file*", 13) == 0 || strncmp(line, "*Error", 6) == 0) {
                    /* Leave an empty file in place of binary or errored files */
                    skip_file = 1;
                    if (out) {
                        fclose(out);
                        out = NULL;
                    }
                    continue;
                }
                fputs(line, out);
            }
        }
    }

    if (out)
        fclose(out);
    fclose(in);
    return 0;
}

