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

// Static variables for split output options
static int split_enabled = 0;
static size_t split_limit_bytes = 18 * 1024 * 1024; // default 18 MB

/**
 * @brief Sets the options for splitting the output into multiple files.
 *
 * @param enabled Non-zero to enable splitting.
 * @param limit_mb Maximum size per file in megabytes.
 */
void set_split_options(int enabled, double limit_mb) {
    split_enabled = enabled;
    split_limit_bytes = (size_t)(limit_mb * 1024 * 1024);
}

/**
 * @brief Prints the documentation statistics to the terminal.
 *
 * Outputs the final token statistics and output file location.
 *
 * @param output_path The path of the output file.
 * @param info Pointer to the DocumentInfo structure containing statistics.
 */
static void print_terminal_stats(const char *output_path, const DocumentInfo *info) {
    printf("\n✨ Directory documentation complete!\n");
    printf("📝 Output: %s\n", output_path);
    printf("📊 Stats:\n");
    printf("   - Total Tokens: %zu\n", info->total_tokens);
    printf("   - Total Size: %.2f MB\n", (double)info->total_size / (1024 * 1024));
}

/**
 * @brief Writes the directory tree structure into the output file.
 *
 * Iterates over the FileList and prints a visual tree along with updating token statistics.
 *
 * @param out The output file stream.
 * @param list Pointer to the FileList containing file and directory entries.
 * @param info Pointer to the DocumentInfo structure for updating statistics.
 */
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

/**
 * @brief Writes the content of a file into the output stream using fenced code blocks.
 *
 * Checks whether the file is binary or text, then writes the file content along with language annotation,
 * and updates the token statistics.
 *
 * @param out The output file stream.
 * @param path The path to the file whose content is to be written.
 * @param info Pointer to the DocumentInfo structure for updating statistics.
 */
void write_file_content(FILE *out, const char *path, DocumentInfo *info) {
    // If file is detected as binary OR its extension indicates a binary file, do not print its contents.
    if (is_binary_file(path) || !is_text_file_by_extension(path)) {
        const char *binary_text = "*Binary file*\n";
        fprintf(out, "%s", binary_text);
        calculate_token_stats(binary_text, info);
        
        char size_text[100];
        snprintf(size_text, sizeof(size_text), "- Size: %s\n", get_file_size(path));
        fprintf(out, "%s", size_text);
        calculate_token_stats(size_text, info);
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
    const char *lang = get_language_from_extension(path);
    
    // Write opening fence without an extra space before the language annotation.
    for (int i = 0; i < fence_count; i++) {
        fputc('`', out);
    }
    if (strlen(lang) > 0) {
        fprintf(out, "%s", lang);
    }
    fputc('\n', out);
    
    fprintf(out, "%s", content);
    calculate_token_stats(content, info);
    
    if (content_size > 0 && content[content_size - 1] != '\n') {
        fprintf(out, "\n");
    }
    
    // Write closing fence.
    for (int i = 0; i < fence_count; i++) {
        fputc('`', out);
    }
    fprintf(out, "\n");
    
    free(content);
}

/**
 * @brief Finalizes the output file by prepending a header and handling file splitting if required.
 *
 * Reads back the generated content, prepends a documentation summary header with token statistics,
 * and if splitting is enabled and necessary, splits the file into multiple parts.
 *
 * @param out_path The output file path.
 * @param info Pointer to the DocumentInfo structure with computed statistics.
 * @return int 0 on success, non-zero on failure.
 */
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
    
    // Prepend documentation summary header to the content
    char header[1024];
    if (split_enabled) {
        snprintf(header, sizeof(header),
            "# Documentation Summary\n\n"
            "The output is a Markdown document summarizing a directory’s structure and file contents. It begins with token and size statistics, followed by a hierarchical view of the directory layout. For each file (unless omitted in structure-only mode), its contents are included in fenced code blocks with optional language annotations and metadata like file size, forming a complete, self-contained reference.\n\n"
            "Note: This document has been split into multiple parts due to size limitations.\n\n"
            "Token Size: %zu\n\n", info->total_tokens);
    } else {
        snprintf(header, sizeof(header),
            "# Documentation Summary\n\n"
            "The output is a Markdown document summarizing a directory’s structure and file contents. It begins with token and size statistics, followed by a hierarchical view of the directory layout. For each file (unless omitted in structure-only mode), its contents are included in fenced code blocks with optional language annotations and metadata like file size, forming a complete, self-contained reference.\n\n"
            "Token Size: %zu\n\n", info->total_tokens);
    }
    
    size_t header_len = strlen(header);
    size_t new_size = header_len + strlen(file_content) + 1;
    char *new_content = malloc(new_size);
    strcpy(new_content, header);
    strcat(new_content, file_content);
    free(file_content);
    
    // If split was not explicitly requested and the content is large, prompt interactively.
    if (!split_enabled && new_size > split_limit_bytes) {
        double size_mb = new_size / (1024.0 * 1024.0);
        printf("⏳ The generated documentation is estimated to be %.2f MB.\n", size_mb);
        printf("Choose an option:\n");
        printf("  [S] Split output into multiple files (default limit: %.2f MB)\n", split_limit_bytes / (1024.0 * 1024.0));
        printf("  [B] Build structure only (skip file contents)\n");
        printf("  [C] Continue as is (do not split)\n");
        printf("  [Q] Quit creation\n");
        printf("Enter your choice [S/B/C/Q]: ");
        fflush(stdout);
        
        char choice[16];
        if (!fgets(choice, sizeof(choice), stdin)) {
            free(new_content);
            return 1;
        }
        
        // Remove trailing newline if any
        choice[strcspn(choice, "\r\n")] = '\0';
        
        if (choice[0] == 'S' || choice[0] == 's') {
            // Ask for new limit in MB (offer default)
            printf("Enter maximum size in MB for each split file (default %.2f MB): ", split_limit_bytes / (1024.0 * 1024.0));
            fflush(stdout);
            char input[32];
            if (fgets(input, sizeof(input), stdin)) {
                // Remove trailing newline
                input[strcspn(input, "\r\n")] = '\0';
                if (strlen(input) > 0) {
                    double new_limit = atof(input);
                    if (new_limit > 0) {
                        split_limit_bytes = (size_t)(new_limit * 1024 * 1024);
                    } else {
                        printf("Invalid input. Using default split limit.\n");
                    }
                }
            }
            split_enabled = 1;
        } else if (choice[0] == 'B' || choice[0] == 'b') {
            // Build structure only: remove file contents by truncating at "## Contents"
            char *contents_pos = strstr(new_content, "\n## Contents");
            if (contents_pos) {
                *contents_pos = '\0';
                printf("✅ Building structure only. File contents will be omitted.\n");
            } else {
                printf("Structure only marker not found. Proceeding without changes.\n");
            }
            split_enabled = 0;
        } else if (choice[0] == 'C' || choice[0] == 'c') {
            // Continue as is without splitting
            split_enabled = 0;
        } else if (choice[0] == 'Q' || choice[0] == 'q') {
            // Quit creation
            printf("Creation cancelled by user.\n");
            free(new_content);
            remove(out_path);
            return 1;
        } else {
            printf("Unrecognized choice. Continuing as is without splitting.\n");
            split_enabled = 0;
        }
    }
    
    // Write back to the original output file (temporary)
    FILE *out_final = fopen(out_path, "w");
    if (!out_final) {
        fprintf(stderr, "Error: Cannot reopen output file '%s' for writing\n", out_path);
        free(new_content);
        return 1;
    }
    fputs(new_content, out_final);
    fclose(out_final);

    // If splitting is enabled, split new_content into multiple files based on split_limit_bytes.
    if (split_enabled) {
        size_t total_size = strlen(new_content);
        size_t part_count = total_size / split_limit_bytes;
        if (total_size % split_limit_bytes != 0) {
            part_count++;
        }
        
        for (size_t i = 0; i < part_count; i++) {
            // Create new file name: original name with _part<number> suffix before extension (if any)
            char part_filename[MAX_PATH_LEN];
            char *dot = strrchr((char *)out_path, '.');
            if (dot) {
                size_t basename_len = dot - out_path;
                snprintf(part_filename, sizeof(part_filename), "%.*s_part%zu%s", (int)basename_len, out_path, i+1, dot);
            } else {
                snprintf(part_filename, sizeof(part_filename), "%s_part%zu", out_path, i+1);
            }
            
            FILE *part_file = fopen(part_filename, "w");
            if (!part_file) {
                fprintf(stderr, "Error: Cannot create split output file '%s'\n", part_filename);
                free(new_content);
                return 1;
            }
            
            size_t offset = i * split_limit_bytes;
            size_t bytes_to_write = split_limit_bytes;
            if (offset + bytes_to_write > total_size) {
                bytes_to_write = total_size - offset;
            }
            fwrite(new_content + offset, 1, bytes_to_write, part_file);
            fclose(part_file);
        }
        
        printf("✅ Output successfully split into %zu parts.\n", part_count);
        // Remove the original unsplit output file.
        remove(out_path);
    }
    
    free(new_content);
    return 0;
}

/**
 * @brief Main documentation generation function.
 *
 * Scans the specified directory, builds the structure and file content sections,
 * writes them to an output file, and finalizes the output (including splitting if necessary).
 *
 * @param input_dir The directory to document.
 * @param output_file The output file path (or NULL for default).
 * @param flags Flags controlling the documentation generation (e.g., IGNORE_GITIGNORE, STRUCTURE_ONLY).
 * @return int 0 on success, non-zero on failure.
 */
int document_directory(const char *input_dir, const char *output_file, int flags) {
    GitignoreList gitignore = {0};
    if (!(flags & IGNORE_GITIGNORE)) {
        load_gitignore(input_dir, &gitignore);
    }
    
    char *out_path = output_file ? (char *)output_file : get_default_output(input_dir);
    
    fprintf(stderr, "⏳ Scanning directory '%s'...\n", input_dir);
    FileList files;
    init_file_list(&files);
    
    bool success = scan_directory(input_dir, NULL, &files, 0, (!(flags & IGNORE_GITIGNORE)) ? &gitignore : NULL, flags);
    /* 
     * If scanning did not add any files but the directory itself is non-empty,
     * warn the user that all files have been ignored.
     */
    if (!success) {
        DIR *d = opendir(input_dir);
        int file_count = 0;
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL) {
                if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                    file_count++;
            }
            closedir(d);
        }
        if (file_count > 0) {
            fprintf(stderr, "Warning: All files in directory '%s' were ignored by .gitignore.\n", input_dir);
            /* Continue with an empty file list */
        } else {
            fprintf(stderr, "Error: No files or folders found in directory '%s'\n", input_dir);
            free_file_list(&files);
            free_gitignore(&gitignore);
            return 1;
        }
    }
    
    fprintf(stderr, "✅ Directory scan complete. Found %zu entries.\n", files.count);
    
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
    fprintf(stderr, "⏳ Generating directory structure...\n");
    write_tree_structure(out, &files, &info);
    
    if (!(flags & STRUCTURE_ONLY)) {
        const char *contents_header = "\n## Contents\n\n";
        fprintf(out, "%s", contents_header);
        calculate_token_stats(contents_header, &info);
        fprintf(stderr, "⏳ Adding file contents...\n");
        
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
