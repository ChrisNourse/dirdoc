// dirdoc.c
#include "cosmopolitan.h"
#include "dirdoc.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs("Usage: dirdoc <directory> [output.md]\n", stderr);
    return 1;
  }

  const char *input_dir = argv[1];
  const char *output_file = argc > 2 ? argv[2] : NULL;
  return document_directory(input_dir, output_file);
}
