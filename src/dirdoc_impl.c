// dirdoc_impl.c
#include "dirdoc.h"
#include "cosmopolitan.h"

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

static int compare_entries(const void *a, const void *b) {
    const FileEntry *fa = a;
    const FileEntry *fb = b;
    
    if (fa->depth != fb->depth)
        return fa->depth - fb->depth;
        
    if (fa->is_dir != fb->is_dir)
        return fb->is_dir - fa->is_dir;
        
    return strcmp(fa->path, fb->path);
}

static void scan_directory(const char *dir_path, const char *rel_path, 
                         FileList *list, int depth) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        
        char full_path[MAX_PATH_LEN];
        char rel_entry_path[MAX_PATH_LEN];
        
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        snprintf(rel_entry_path, sizeof(rel_entry_path), "%s%s%s", 
                rel_path ? rel_path : "", rel_path ? "/" : "", entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            bool is_dir = S_ISDIR(st.st_mode);
            add_file_entry(list, rel_entry_path, is_dir, depth);
            
            if (is_dir) {
                scan_directory(full_path, rel_entry_path, list, depth + 1);
            }
        }
    }
    
    closedir(dir);
}

char *get_default_output(const char *input_dir) {
    static char buffer[MAX_PATH_LEN];
    const char *base = strrchr(input_dir, '/');
    if (!base) base = input_dir;
    else base++;
    snprintf(buffer, sizeof(buffer), "%s_documentation.md", base);
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
    // TODO: Implement SHA-256 hash calculation
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

void calculate_token_stats(const char *str, DocumentStats *stats) {
    bool in_word = false;
    const char *p = str;
    
    while (*p) {
        if (isspace(*p) || *p == '\n' || *p == '\t') {
            if (in_word) {
                stats->word_count++;
                stats->token_count++;
                in_word = false;
            }
        } else {
            if (!in_word) {
                in_word = true;
            }
        }
        
        if (*p == '#' || *p == '*' || *p == '_' || *p == '`' || *p == '[' || 
            *p == ']' || *p == '(' || *p == ')' || *p == '|' || *p == '-') {
            stats->token_count++;
        }
        
        stats->char_count++;
        p++;
    }
    
    if (in_word) {
        stats->word_count++;
        stats->token_count++;
    }
}

void write_file_content(FILE *out, const char *path, DocumentStats *stats) {
    if (is_binary_file(path)) {
        const char *binary_text = "*Binary file*\n";
        fprintf(out, "%s", binary_text);
        calculate_token_stats(binary_text, stats);
        
        char size_text[100];
        snprintf(size_text, sizeof(size_text), "- Size: %s\n", get_file_size(path));
        fprintf(out, "%s", size_text);
        calculate_token_stats(size_text, stats);
        
        char hash_text[100];
        snprintf(hash_text, sizeof(hash_text), "- SHA-256: %s\n", get_file_hash(path));
        fprintf(out, "%s", hash_text);
        calculate_token_stats(hash_text, stats);
        return;
    }
    
    fprintf(out, "%s\n", FENCE);
    calculate_token_stats(FENCE "\n", stats);
    
    FILE *f = fopen(path, "r");
    if (!f) {
        const char *error_text = "*Error reading file*\n";
        fprintf(out, "%s", error_text);
        calculate_token_stats(error_text, stats);
        return;
    }
    
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), f)) {
        fputs(buffer, out);
        calculate_token_stats(buffer, stats);
    }
    
    fprintf(out, "%s\n", FENCE);
    calculate_token_stats(FENCE "\n", stats);
    fclose(f);
}

int document_directory(const char *input_dir, const char *output_file) {
    char *out_path = output_file ? (char *)output_file : get_default_output(input_dir);
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot create output file %s\n", out_path);
        return 1;
    }
    
    DocumentStats stats = {0};
    
    // Write header
    const char *header = "# Directory Documentation: ";
    fprintf(out, "%s%s\n\n", header, strrchr(input_dir, '/') + 1);
    calculate_token_stats(header, &stats);
    
    const char *structure_header = "## Structure\n\n";
    fprintf(out, "%s", structure_header);
    calculate_token_stats(structure_header, &stats);
    
    // Scan directory
    FileList files = {0};
    init_file_list(&files);
    scan_directory(input_dir, NULL, &files, 0);
    qsort(files.entries, files.count, sizeof(FileEntry), compare_entries);
    
    // Write structure
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
        calculate_token_stats(line, &stats);
    }
    
    // Write contents
    const char *contents_header = "\n## Contents\n\n";
    fprintf(out, "%s", contents_header);
    calculate_token_stats(contents_header, &stats);
    
    for (size_t i = 0; i < files.count; i++) {
        FileEntry *entry = &files.entries[i];
        if (!entry->is_dir) {
            char full_path[MAX_PATH_LEN];
            char header[MAX_PATH_LEN + 10];
            snprintf(full_path, sizeof(full_path), "%s/%s", input_dir, entry->path);
            snprintf(header, sizeof(header), "### 📄 %s\n\n", entry->path);
            
            fprintf(out, "%s", header);
            calculate_token_stats(header, &stats);
            write_file_content(out, full_path, &stats);
            fprintf(out, "\n");
        }
    }
    
    // Write stats
    fprintf(out, "\n## Document Statistics\n");
    fprintf(out, "- Characters: %zu\n", stats.char_count);
    fprintf(out, "- Words: %zu\n", stats.word_count);
    fprintf(out, "- Tokens: %zu\n", stats.token_count);
    
    free_file_list(&files);
    fclose(out);
    return 0;
}