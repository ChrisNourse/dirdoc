#include <cosmo.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dirdoc.h"
#include "writer.h"  // Include writer.h to set split options

static void print_help() {
    printf("Usage: dirdoc [OPTIONS] <directory>\n\n"
           "Options:\n"
           "  -h,   --help               Show this help message.\n"
           "  -o,   --output <file>      Specify output file (default: directory_documentation.md).\n"
           "  -ngi, --no-gitignore       Ignore .gitignore file.\n"
           "  -s,   --structure-only     Generate structure only (skip file contents).\n"
           "  -sp,  --split              Enable split output. Optionally, use -l/--limit to specify maximum file size in MB (default: 18).\n"
           "  -l,   --limit <limit>      Set maximum file size in MB for each split file (used with -sp).\n"
           "  -ig,  --include-git        Include .git folders in documentation (default: ignored).\n\n"
           "Examples:\n"
           "  dirdoc /path/to/dir\n"
           "  dirdoc -o custom.md /path/to/dir\n"
           "  dirdoc --no-gitignore /path/to/dir\n"
           "  dirdoc --structure-only /path/to/dir\n"
           "  dirdoc -sp /path/to/dir\n"
           "  dirdoc -sp -l 10 /path/to/dir\n"
           "  dirdoc --include-git /path/to/dir\n");
}

char *get_default_output(const char *input_dir) {
    static char buffer[MAX_PATH_LEN];
    snprintf(buffer, sizeof(buffer), "directory_documentation.md");
    return buffer;
}

int main(int argc, char *argv[]) {
    const char *input_dir = NULL;
    const char *output_file = NULL;
    int flags = 0;
    double split_limit_mb = 18.0; // Default split limit in MB

    if (argc < 2) {
        print_help();
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            print_help();
            return 0;
        } else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0)) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: --output requires a filename argument.\n");
                return 1;
            }
        } else if ((strcmp(argv[i], "--no-gitignore") == 0) || (strcmp(argv[i], "-ngi") == 0)) {
            flags |= IGNORE_GITIGNORE;
        } else if ((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--structure-only") == 0)) {
            flags |= STRUCTURE_ONLY;
        } else if ((strcmp(argv[i], "-sp") == 0) || (strcmp(argv[i], "--split") == 0)) {
            flags |= SPLIT_OUTPUT;
            // Check if the next arguments specify a limit with -l or --limit
            if (i + 2 < argc &&
                ((strcmp(argv[i+1], "-l") == 0) || (strcmp(argv[i+1], "--limit") == 0))) {
                split_limit_mb = atof(argv[i+2]);
                if (split_limit_mb <= 0) {
                    fprintf(stderr, "Error: Invalid split limit specified. Using default of 18 MB.\n");
                    split_limit_mb = 18.0;
                }
                i += 2;
            }
        } else if ((strcmp(argv[i], "-l") == 0) || (strcmp(argv[i], "--limit") == 0)) {
            // If -l is provided without -sp, warn and ignore limit
            fprintf(stderr, "Warning: -l/--limit specified without -sp/--split. Ignoring limit.\n");
            if (i + 1 < argc) {
                i++; // Skip the limit value
            }
        } else if ((strcmp(argv[i], "-ig") == 0) || (strcmp(argv[i], "--include-git") == 0)) {
            flags |= INCLUDE_GIT;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_help();
            return 1;
        } else {
            if (input_dir) {
                fprintf(stderr, "Error: Multiple directories specified.\n");
                print_help();
                return 1;
            }
            input_dir = argv[i];
        }
    }

    if (!input_dir) {
        fprintf(stderr, "Error: No directory specified.\n");
        print_help();
        return 1;
    }

    // Set split options in writer module if SPLIT_OUTPUT flag is enabled.
    if (flags & SPLIT_OUTPUT) {
        set_split_options(1, split_limit_mb);
    }

    // Call document_directory from the writer module.
    return document_directory(input_dir, output_file, flags);
}
