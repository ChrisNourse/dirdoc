#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "writer.h"
#include "scanner.h"
#include "stats.h"
#include "gitignore.h"
#include "dirdoc.h"

static void print_terminal_stats(const char *output_path, const DocumentInfo *info) {
    printf("\n✨ Directory documentation complete!\n");
    printf("📝 Output: %s\n", output_path);
    printf("📊 Stats:\n");
    printf("   - Total Tokens: %zu\n", info->total_tokens);
    printf("   - Total Size: %.2f MB\n", (double)info->total_size / (1024 * 1024));
}

void write_tree_structure(FILE *out, FileList *list, DocumentInfo *info) {
    bool *has_sibling = calloc(MAX_PATH_LEN, sizeof(bool));
    for (size_t i = 0; i < list->count; i++) {
        FileEntry *entry = &list->entries[i];
        size_t next_depth = (i + 1 < list->count) ? list->entries[i + 1].depth : 0;
        
        for (size_t d = 0; d < (size_t)entry->depth; d++) {
            if (d == (size_t)entry->depth - 1) {
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
        
        has_sibling[entry->depth] = (next_depth >= (size_t)entry->depth);
    }
    free(has_sibling);
    fprintf(out, "```\n");
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
    const char *lang = get_language_from_extension(path);
    
    for (int i = 0; i < fence_count; i++) {
        fputc('`', out);
    }
    if (strlen(lang) > 0) {
        fputc(' ', out);
        fprintf(out, "%s", lang);
    }
    fputc('\n', out);
    
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

int finalize_output(const char *out_path, DocumentInfo *info) {
    FILE *in = fopen(out_path, "r");
    if (!in) {
        fprintf(stderr, "Error: Cannot reopen output file '%s' for reading\n", out_path);
        return 1;
    }
    fseek(in, 0, SEEK_END);
    long fsize = ftell(in);
    fseek(in, 0, SEEK_SET);
    char *file_content = malloc(fsize + 1);
    if (fread(file_content, 1, fsize, in) != (size_t)fsize) {
        fprintf(stderr, "Error: Reading output file '%s'\n", out_path);
        free(file_content);
        fclose(in);
        return 1;
    }
    file_content[fsize] = '\0';
    fclose(in);
    
    FILE *out_final = fopen(out_path, "w");
    if (!out_final) {
        fprintf(stderr, "Error: Cannot reopen output file '%s' for writing\n", out_path);
        free(file_content);
        return 1;
    }
    fprintf(out_final, "Token Size: %zu\n\n", info->total_tokens);
    fputs(file_content, out_final);
    fclose(out_final);
    free(file_content);
    return 0;
}

int document_directory(const char *input_dir, const char *output_file, int flags) {
    GitignoreList gitignore = {0};
    if (!(flags & IGNORE_GITIGNORE)) {
        load_gitignore(input_dir, &gitignore);
    }
    
    char *out_path = output_file ? (char *)output_file : get_default_output(input_dir);
    
    FileList files;
    init_file_list(&files);
    
    bool success = scan_directory(input_dir, NULL, &files, 0, (!(flags & IGNORE_GITIGNORE)) ? &gitignore : NULL);
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
    
    if (!(flags & STRUCTURE_ONLY)) {
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
    }
    
    fclose(out);
    free_file_list(&files);
    free_gitignore(&gitignore);
    
    if (finalize_output(out_path, &info) != 0) {
        return 1;
    }
    
    print_terminal_stats(out_path, &info);
    return 0;
}
