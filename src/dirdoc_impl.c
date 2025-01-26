// dirdoc_impl.c
#include "dirdoc.h"
#include "cosmopolitan.h"

typedef struct {
    char **patterns;
    size_t count;
} GitignoreList;

typedef struct FileList {
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

static bool match_gitignore(const char *path, GitignoreList *gitignore) {
    for (size_t i = 0; i < gitignore->count; i++) {
        if (strstr(path, gitignore->patterns[i]) != NULL) {
            return true;
        }
    }
    return false;
}

static void load_gitignore(const char *dir_path, GitignoreList *gitignore) {
    char gitignore_path[MAX_PATH_LEN];
    snprintf(gitignore_path, sizeof(gitignore_path), "%s/.gitignore", dir_path);
    
    FILE *f = fopen(gitignore_path, "r");
    if (!f) return;
    
    gitignore->patterns = malloc(sizeof(char*) * 100);
    gitignore->count = 0;
    
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        line[strcspn(line, "\n")] = 0;
        gitignore->patterns[gitignore->count++] = strdup(line);
    }
    
    fclose(f);
}

static void free_gitignore(GitignoreList *gitignore) {
    for (size_t i = 0; i < gitignore->count; i++) {
        free(gitignore->patterns[i]);
    }
    free(gitignore->patterns);
}

static int compare_entries(const void *a, const void *b) {
    const FileEntry *fa = a;
    const FileEntry *fb = b;
    
    if (fa->depth != fb->depth)
        return fa->depth - fb->depth;
        
    if (fa->is_dir != fb->is_dir)
        return fb->is_dir - fa->is_dir;
        
    return strcmp(fa->path, fb->path);
}

static bool scan_directory(const char *dir_path, const char *rel_path, 
                         FileList *list, int depth, GitignoreList *gitignore) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: Directory '%s' does not exist\n", dir_path);
        return false;
    }
    
    bool has_entries = false;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".gitignore") != 0 && entry->d_name[0] == '.') continue;
        
        char full_path[MAX_PATH_LEN];
        char rel_entry_path[MAX_PATH_LEN];
        
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        snprintf(rel_entry_path, sizeof(rel_entry_path), "%s%s%s", 
                rel_path ? rel_path : "", rel_path ? "/" : "", entry->d_name);
                
        if (strcmp(entry->d_name, ".gitignore") != 0 && match_gitignore(rel_entry_path, gitignore)) {
            continue;
        }
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            bool is_dir = S_ISDIR(st.st_mode);
            add_file_entry(list, rel_entry_path, is_dir, depth);
            
            if (is_dir) {
                has_entries |= scan_directory(full_path, rel_entry_path, list, depth + 1, gitignore);
            }
        }
    }
    
    closedir(dir);
    return has_entries || list->count > 0;
}

char *get_default_output(const char *input_dir) {
    static char buffer[MAX_PATH_LEN];
    snprintf(buffer, sizeof(buffer), "directory_documentation.md");
    return buffer;
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
    if (stat(path, &st) != 0) return "unknown";
    
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

static inline int max(int a, int b) {
    return a > b ? a : b;
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
    return max_count;
}

void calculate_token_stats(const char *str, DocumentInfo *info) {
    bool in_word = false;
    const char *p = str;
    
    while (*p) {
        if (isspace(*p) || *p == '\n' || *p == '\t') {
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
    
    int max_backticks = count_max_backticks(content);
    int fence_count = max(3, max_backticks + 1);
    
    for (int i = 0; i < fence_count; i++) {
        fputc('`', out);
    }
    fprintf(out, "\n");
    
    fprintf(out, "%s", content);
    calculate_token_stats(content, info);
    
    if (content[content_size-1] != '\n') {
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

int document_directory(const char *input_dir, const char *output_file) {
    GitignoreList gitignore = {0};
    load_gitignore(input_dir, &gitignore);
    
    char *out_path = output_file ? (char *)output_file : get_default_output(input_dir);
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot create output file %s\n", out_path);
        return 1;
    }
    
    DocumentInfo info = {0};
    
    const char *header = "# Directory Documentation: ";
    fprintf(out, "%s%s\n\n", header, strrchr(input_dir, '/') + 1);
    calculate_token_stats(header, &info);
    
    const char *structure_header = "## Structure\n\n";
    fprintf(out, "%s", structure_header);
    calculate_token_stats(structure_header, &info);
    
    FileList files = {0};
    init_file_list(&files);
    
    if (!scan_directory(input_dir, NULL, &files, 0, &gitignore)) {
        fprintf(stderr, "Error: No files or folders found in directory '%s'\n", input_dir);
        free_gitignore(&gitignore);
        return 1;
    }
    
    qsort(files.entries, files.count, sizeof(FileEntry), compare_entries);
    
    for (size_t i = 0; i < files.count; i++) {
        FileEntry *entry = &files.entries[i];
        char line[MAX_PATH_LEN + 10];
        for (int j = 0; j < entry->depth; j++) {
            fprintf(out, "  ");
        }
        snprintf(line, sizeof(line), "- %s %s%s\n", 
                entry->is_dir ? "📁" : "📄",
                entry->path,
                entry->is_dir ? "/" : "");
        fprintf(out, "%s", line);
        calculate_token_stats(line, &info);
    }
    
    const char *contents_header = "\n## Contents\n\n";
    fprintf(out, "%s", contents_header);
    calculate_token_stats(contents_header, &info);
    
    for (size_t i = 0; i < files.count; i++) {
        FileEntry *entry = &files.entries[i];
        if (!entry->is_dir) {
            char full_path[MAX_PATH_LEN];
            char header[MAX_PATH_LEN + 10];
            snprintf(full_path, sizeof(full_path), "%s/%s", input_dir, entry->path);
            snprintf(header, sizeof(header), "### 📄 %s\n\n", entry->path);
            
            fprintf(out, "%s", header);
            calculate_token_stats(header, &info);
            write_file_content(out, full_path, &info);
            fprintf(out, "\n");
        }
    }
    
    print_terminal_stats(out_path, &info);
    free_file_list(&files);
    free_gitignore(&gitignore);
    fclose(out);
    return 0;
}