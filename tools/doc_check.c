#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

static int lstrip_index(const char *s) {
    int i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    return i;
}

static int has_doc_comment(char **lines, int idx) {
    for (int j = idx - 1; j >= 0; j--) {
        const char *line = lines[j];
        int i = lstrip_index(line);
        if (line[i] == '\0') continue;
        if (line[i] == '*' ||
            (line[i] == '/' && line[i+1] == '/') ||
            line[i] == '#') {
            continue;
        }
        return strncmp(line + i, "/**", 3) == 0;
    }
    return 0;
}

static int check_file(const char *path, regex_t *re) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror(path);
        return 1;
    }
    char **lines = NULL;
    size_t count = 0, capacity = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t n;
    while ((n = getline(&line, &len, fp)) != -1) {
        if (count >= capacity) {
            capacity = capacity ? capacity * 2 : 128;
            lines = realloc(lines, capacity * sizeof(char *));
            if (!lines) {
                perror("realloc");
                fclose(fp);
                free(line);
                return 1;
            }
        }
        lines[count++] = strdup(line);
    }
    free(line);
    fclose(fp);

    int missing = 0;
    for (size_t i = 0; i < count; i++) {
        if (regexec(re, lines[i], 0, NULL, 0) == 0) {
            char *s = lines[i];
            int off = lstrip_index(s);
            if (strncmp(s + off, "struct ", 7) == 0 ||
                strncmp(s + off, "enum ", 5) == 0 ||
                strncmp(s + off, "class ", 6) == 0) {
                continue;
            }
            if (!has_doc_comment(lines, (int)i)) {
                printf("%s:%zu: missing Doxygen comment\n", path, i + 1);
                missing = 1;
            }
        }
    }

    for (size_t i = 0; i < count; i++) free(lines[i]);
    free(lines);
    return missing;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <files...>\n", argv[0]);
        return 1;
    }
    regex_t re;
    const char *pattern = "^[A-Za-z_][A-Za-z0-9_[:space:]*]*\\([^;]*\\)[[:space:]]*\\{";
    if (regcomp(&re, pattern, REG_EXTENDED)) {
        fprintf(stderr, "Failed to compile regex\n");
        return 1;
    }
    int missing = 0;
    for (int i = 1; i < argc; i++) {
        missing |= check_file(argv[i], &re);
    }
    regfree(&re);
    return missing ? 1 : 0;
}
