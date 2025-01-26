#include <cosmo.h>    // Cosmopolitan environment
#include <ctype.h>    // isspace(), etc.
#include <dirent.h>   // DIR, opendir(), readdir(), closedir()
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>    // printf(), fprintf(), fopen(), fclose(), etc.
#include <stdlib.h>   // malloc(), free(), realloc(), system(), mkdtemp(), etc.
#include <string.h>   // strcmp(), strrchr(), strlen(), etc.
#include <sys/stat.h> // struct stat, stat(), S_ISDIR
#include <unistd.h>   // for close() or access(), etc.

#include "dirdoc.h"

void print_help() {
    printf("Usage: dirdoc [options] <directory>\n\n"
           "Options:\n"
           "  -h, --help        Show this help message\n"
           "  -o, --output      Specify output file (default: directory_documentation.md)\n"
           "  --no-gitignore    Ignore .gitignore file\n"
           "\nExamples:\n"
           "  dirdoc /path/to/dir\n"
           "  dirdoc -o custom.md /path/to/dir\n"
           "  dirdoc --no-gitignore /path/to/dir\n");
}

int main(int argc, char *argv[]) {
    const char *input_dir = NULL;
    const char *output_file = NULL;
    int flags = 0;

    // Show help if no arguments
    if (argc < 2) {
        print_help();
        return 0;
    }

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
        else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
            output_file = argv[++i];
        }
        else if (strcmp(argv[i], "--no-gitignore") == 0) {
            flags |= IGNORE_GITIGNORE;
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help();
            return 1;
        }
        else {
            if (input_dir) {
                fprintf(stderr, "Multiple directories specified\n");
                print_help();
                return 1;
            }
            input_dir = argv[i];
        }
    }

    if (!input_dir) {
        fprintf(stderr, "No directory specified\n");
        print_help();
        return 1;
    }

    return document_directory(input_dir, output_file, flags);
}