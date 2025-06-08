#include <cosmo.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include "dirdoc.h"
#include "writer.h"  // Include writer.h to set split options
#include "reconstruct.h"

#if !defined(UNIT_TEST)
/**
 * @brief Prints the help/usage information to stdout.
 */
static void print_help() {
    printf("Usage: dirdoc [OPTIONS] <directory>\n\n"
           "Options:\n"
           "  -h,   --help               Show this help message.\n"
           "  -o,   --output <file>      Specify output file (default: <folder>_documentation.md, where <folder> is the name of the input directory).\n"
           "  -ngi, --no-gitignore       Ignore .gitignore file; however, extra ignore patterns provided with --ignore will still be applied.\n"
           "  -s,   --structure-only     Generate structure only (skip file contents).\n"
           "  -sp,  --split              Enable split output. Optionally, use -l/--limit to specify maximum file size in MB (default: 18).\n"
           "  -l,   --limit <limit>      Set maximum file size in MB for each split file (used with -sp).\n"
           "  -ig,  --include-git        Include .git folders in documentation (default: ignored).\n"
           "  --ignore <pattern>         Ignore files matching the specified pattern (supports wildcards). Can be specified multiple times.\n"
           "  -rc,  --reconstruct        Reconstruct a directory from a dirdoc markdown. Use -o to specify the output directory.\n\n"
           "Examples:\n"
           "  dirdoc /path/to/dir\n"
           "  dirdoc -o custom.md /path/to/dir\n"
           "  dirdoc --no-gitignore /path/to/dir\n"
           "  dirdoc --structure-only /path/to/dir\n"
           "  dirdoc -sp /path/to/dir\n"
           "  dirdoc -sp -l 10 /path/to/dir\n"
           "  dirdoc --include-git /path/to/dir\n"
           "  dirdoc --ignore \"*.tmp\" /path/to/dir\n"
           "  dirdoc --ignore \"*.log\" --ignore \"secret.txt\" /path/to/dir\n"
           "  dirdoc --ignore \"temp/\" /path/to/dir          # Ignore the entire temp directory\n");
}

/**
 * @brief Main entry point for the dirdoc application.
 *
 * Parses command-line arguments, sets up options (including split options and extra ignore patterns),
 * and calls the document_directory function to generate the documentation.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int 0 on success, non-zero on failure.
 */
int main(int argc, char *argv[]) {
    const char *input_dir = NULL;
    const char *output_file = NULL;
    int flags = 0;
    double split_limit_mb = 18.0; // Default split limit in MB
    int reconstruct_mode = 0;

    #define MAX_IGNORE_PATTERNS 64
    char *ignore_patterns[MAX_IGNORE_PATTERNS];
    int ignore_patterns_count = 0;

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
        } else if ((strcmp(argv[i], "--reconstruct") == 0) || (strcmp(argv[i], "-rc") == 0)) {
            reconstruct_mode = 1;
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
        } else if (strcmp(argv[i], "--ignore") == 0) {
            if (i + 1 < argc) {
                if (ignore_patterns_count < MAX_IGNORE_PATTERNS) {
                    ignore_patterns[ignore_patterns_count++] = argv[++i];
                } else {
                    fprintf(stderr, "Error: Too many ignore patterns specified.\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "Error: --ignore requires a pattern argument.\n");
                return 1;
            }
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
        fprintf(stderr, "Error: No input path specified.\n");
        print_help();
        return 1;
    }

    if (reconstruct_mode) {
        const char *out_dir = output_file ? output_file : ".";
        return reconstruct_from_markdown(input_dir, out_dir);
    }

    // Set split options in writer module if SPLIT_OUTPUT flag is enabled.
    if (flags & SPLIT_OUTPUT) {
        set_split_options(1, split_limit_mb);
    }

    // Set extra ignore patterns for files (if any)
    if (ignore_patterns_count > 0) {
        set_extra_ignore_patterns(ignore_patterns, ignore_patterns_count);
    }

    int result = document_directory(input_dir, output_file, flags);

    free_extra_ignore_patterns();

    return result;
}
#endif  /* !defined(UNIT_TEST) */

/**
 * @brief Returns a default output filename based on the input directory.
 *
 * Generates a markdown filename based on the provided directory name. The
 * returned string is dynamically allocated and must be freed by the caller.
 *
 * @param input_dir The path of the directory being documented.
 * @return char* Newly allocated output filename.
 */
char *get_default_output(const char *input_dir) {
    const char *default_name = "directory_documentation.md";
    if (input_dir == NULL || strlen(input_dir) == 0) {
        return strdup(default_name);
    }
    
    char *dirpath = NULL;
    // If the input is "." or "./", resolve it to the current working directory.
    if (strcmp(input_dir, ".") == 0 || strcmp(input_dir, "./") == 0) {
        dirpath = getcwd(NULL, 0);
        if (!dirpath) {
            return strdup(default_name);
        }
    } else {
        // For other cases, duplicate the provided input.
        dirpath = strdup(input_dir);
        if (!dirpath) {
            return strdup(default_name);
        }
    }
    
    // Use basename() to extract the folder name.
    char *base = basename(dirpath);
    size_t len = strlen(base) + strlen("_documentation.md") + 1;
    char *output = (char*)malloc(len); // Explicit cast for C++ compatibility
    if (output) {
        snprintf(output, len, "%s_documentation.md", base);
    } else {
        output = strdup(default_name);
    }
    free(dirpath);
    return output;
}
