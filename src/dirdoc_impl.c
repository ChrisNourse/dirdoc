#include <cosmo.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dirdoc.h"
#include "gitignore.h"

typedef struct {
    FileEntry *entries;
    size_t count;
    size_t capacity;
} FileList;

static void init_file_list(FileList *list) {
    list->capacity = 16;
    list->count = 0;
    list->entries = malloc(list->capacity * sizeof(FileEntry));
}

static void add_file_entry(FileList *list, const char *path, bool is_dir, int depth) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->entries = realloc(list->entries, list->capacity * sizeof(FileEntry));
    }
    
    FileEntry *entry = &list->entries[list->count++];
    entry->path = strdup(path);
    entry->is_dir = is_dir;
    entry->depth = depth;
}

static void free_file_list(FileList *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->entries[i].path);
    }
    free(list->entries);
}

static int compare_entries(const void *a, const void *b) {
    const FileEntry *fa = (const FileEntry *)a;
    const FileEntry *fb = (const FileEntry *)b;
    
    char *parent_a = strdup(fa->path);
    char *parent_b = strdup(fb->path);
    char *last_slash_a = strrchr(parent_a, '/');
    char *last_slash_b = strrchr(parent_b, '/');
    
    if (last_slash_a) *last_slash_a = '\0';
    if (last_slash_b) *last_slash_b = '\0';
    
    int parent_cmp = strcmp(parent_a ? parent_a : "", parent_b ? parent_b : "");
    free(parent_a);
    free(parent_b);
    if (parent_cmp != 0) {
        return parent_cmp;
    }
    
    if (fa->is_dir != fb->is_dir) {
        return (fb->is_dir - fa->is_dir);
    }
    
    return strcmp(fa->path, fb->path);
}

static bool scan_directory(const char *dir_path,
                           const char *rel_path,
                           FileList *list,
                           int depth,
                           const GitignoreList *gitignore)
{
    if (gitignore && rel_path && match_gitignore(rel_path, gitignore)) {
        return false;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: Directory '%s' does not exist or cannot be opened\n", dir_path);
        return false;
    }

    bool has_entries = false;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH_LEN];
        char rel_entry_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        snprintf(rel_entry_path, sizeof(rel_entry_path), "%s%s%s",
                 rel_path ? rel_path : "",
                 rel_path ? "/" : "",
                 entry->d_name);

        if (gitignore && match_gitignore(rel_entry_path, gitignore)) {
            continue;
        }

        struct stat st;
        if (stat(full_path, &st) == 0) {
            bool is_subdir = S_ISDIR(st.st_mode);
            add_file_entry(list, rel_entry_path, is_subdir, depth);
            if (is_subdir) {
                has_entries |= scan_directory(full_path, rel_entry_path, list, depth + 1, gitignore);
            }
        }
    }
    closedir(dir);

    return has_entries || (list->count > 0);
}

static void write_tree_structure(FILE *out, FileList *list, DocumentInfo *info) {
    bool *has_sibling = calloc(MAX_PATH_LEN, sizeof(bool));
    
    for (size_t i = 0; i < list->count; i++) {
        FileEntry *entry = &list->entries[i];
        size_t next_depth = (i + 1 < list->count) ? list->entries[i + 1].depth : 0;
        
        for (size_t d = 0; d < entry->depth; d++) {
            if (d == entry->depth - 1) {
                fprintf(out, "├── ");
            } else if (has_sibling[d]) {
                fprintf(out, "│   ");
            } else {
                fprintf(out, "    ");
            }
        }

        char line[MAX_PATH_LEN + 16];
        snprintf(line, sizeof(line), "%s %s%s\n",
                 entry->is_dir ? "📁" : "📄",
                 strrchr(entry->path, '/') ? strrchr(entry->path, '/') + 1 : entry->path,
                 entry->is_dir ? "/" : "");
        fprintf(out, "%s", line);
        calculate_token_stats(line, info);

        has_sibling[entry->depth] = (next_depth >= entry->depth);
    }
    free(has_sibling);
    fprintf(out, "```\n");
}

bool is_binary_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return true;
    
    unsigned char buf[1024];
    size_t len = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == 0) return true;
    }
    return false;
}

char *get_file_hash(const char *path) {
    static char hash[65];
    strcpy(hash, "hash_not_implemented");
    return hash;
}

char *get_file_size(const char *path) {
    static char size[32];
    struct stat st;
    if (stat(path, &st) != 0) {
        return "unknown";
    }
    
    double size_bytes = st.st_size;
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    
    while (size_bytes >= 1024 && unit < 4) {
        size_bytes /= 1024;
        unit++;
    }
    
    snprintf(size, sizeof(size), "%.2f %s", size_bytes, units[unit]);
    return size;
}

int count_max_backticks(const char *content) {
    int max_count = 0;
    int current_count = 0;
    const char *p = content;
    
    while (*p) {
        if (*p == '`') {
            current_count++;
        } else {
            if (current_count > max_count) {
                max_count = current_count;
            }
            current_count = 0;
        }
        p++;
    }
    if (current_count > max_count) {
        max_count = current_count;
    }
    return max_count;
}

void calculate_token_stats(const char *str, DocumentInfo *info) {
    bool in_word = false;
    const char *p = str;
    
    while (*p) {
        if (isspace((unsigned char)*p) || *p == '\n' || *p == '\t') {
            if (in_word) {
                info->total_tokens++;
                in_word = false;
            }
        } else {
            if (!in_word) {
                in_word = true;
            }
        }
        
        if (*p == '#' || *p == '*' || *p == '_' || *p == '`' || *p == '[' ||
            *p == ']' || *p == '(' || *p == ')' || *p == '|' || *p == '-') {
            info->total_tokens++;
        }
        
        info->total_size++;
        p++;
    }
    if (in_word) {
        info->total_tokens++;
    }
}

void write_file_content(FILE *out, const char *path, DocumentInfo *info) {
    if (is_binary_file(path)) {
        const char *binary_text = "*Binary file*\n";
        fprintf(out, "%s", binary_text);
        calculate_token_stats(binary_text, info);
        
        char size_text[100];
        snprintf(size_text, sizeof(size_text), "- Size: %s\n", get_file_size(path));
        fprintf(out, "%s", size_text);
        calculate_token_stats(size_text, info);
        
        char hash_text[100];
        snprintf(hash_text, sizeof(hash_text), "- SHA-256: %s\n", get_file_hash(path));
        fprintf(out, "%s", hash_text);
        calculate_token_stats(hash_text, info);
        return;
    }
    
    FILE *f = fopen(path, "r");
    if (!f) {
        const char *error_text = "*Error reading file*\n";
        fprintf(out, "%s", error_text);
        calculate_token_stats(error_text, info);
        return;
    }
    
    char *content = malloc(BUFFER_SIZE);
    size_t content_size = 0;
    size_t capacity = BUFFER_SIZE;
    
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), f)) {
        size_t len = strlen(buffer);
        if (content_size + len >= capacity) {
            capacity *= 2;
            content = realloc(content, capacity);
        }
        strcpy(content + content_size, buffer);
        content_size += len;
    }
    fclose(f);
    
    int max_ticks = count_max_backticks(content);
    int fence_count = (max_ticks < 3) ? 3 : (max_ticks + 1);
    
    for (int i = 0; i < fence_count; i++) {
        fputc('`', out);
    }
    fprintf(out, "\n");
    
    fprintf(out, "%s", content);
    calculate_token_stats(content, info);
    
    if (content_size > 0 && content[content_size - 1] != '\n') {
        fprintf(out, "\n");
    }
    
    for (int i = 0; i < fence_count; i++) {
        fputc('`', out);
    }
    fprintf(out, "\n");
    
    free(content);
}

void print_terminal_stats(const char *output_path, const DocumentInfo *info) {
    printf("\n✨ Directory documentation complete!\n");
    printf("📝 Output: %s\n", output_path);
    printf("📊 Stats:\n");
    printf("   - Total Tokens: %zu\n", info->total_tokens);
    printf("   - Total Size: %.2f MB\n", (double)info->total_size / (1024 * 1024));
}

int document_directory(const char *input_dir, const char *output_file, int flags) {
    GitignoreList gitignore = {0};
    if (!(flags & IGNORE_GITIGNORE)) {
        load_gitignore(input_dir, &gitignore);
    }

    char *out_path = output_file ? (char *)output_file : get_default_output(input_dir);

    FileList files;
    init_file_list(&files);

    bool success = scan_directory(input_dir, NULL, &files, 0,
                                  (!(flags & IGNORE_GITIGNORE)) ? &gitignore : NULL);
    if (!success) {
        fprintf(stderr, "Error: No files or folders found in directory '%s'\n", input_dir);
        free_file_list(&files);
        free_gitignore(&gitignore);
        return 1;
    }

    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", out_path);
        free_file_list(&files);
        free_gitignore(&gitignore);
        return 1;
    }

    DocumentInfo info = {0};

    const char *header = "# Directory Documentation: ";
    fprintf(out, "%s%s\n\n", header,
            strrchr(input_dir, '/') ? strrchr(input_dir, '/') + 1 : input_dir);
    calculate_token_stats(header, &info);

    const char *structure_header = "## Structure\n\n";
    fprintf(out, "%s", structure_header);
    calculate_token_stats(structure_header, &info);
    fprintf(out, "```\n");

    qsort(files.entries, files.count, sizeof(FileEntry), compare_entries);
    write_tree_structure(out, &files, &info);

    const char *contents_header = "\n## Contents\n\n";
    fprintf(out, "%s", contents_header);
    calculate_token_stats(contents_header, &info);

    for (size_t i = 0; i < files.count; i++) {
        FileEntry *entry = &files.entries[i];
        if (!entry->is_dir) {
            char full_path[MAX_PATH_LEN];
            snprintf(full_path, sizeof(full_path), "%s/%s", input_dir, entry->path);

            char heading[MAX_PATH_LEN + 16];
            snprintf(heading, sizeof(heading), "### 📄 %s\n\n", entry->path);
            fprintf(out, "%s", heading);
            calculate_token_stats(heading, &info);

            write_file_content(out, full_path, &info);
            fprintf(out, "\n");
        }
    }

    print_terminal_stats(out_path, &info);
    fclose(out);
    free_file_list(&files);
    free_gitignore(&gitignore);
    return 0;
}