#include "tiktoken.h"
#include <stdlib.h>
#include <string.h>

// Define a C-compatible struct for wrapper
typedef struct TiktokenWrapper TiktokenWrapper;

// External functions implemented in tiktoken_cpp.cpp
extern TiktokenWrapper* tiktoken_cpp_get_encoding(const char* encoding_name);
extern int tiktoken_cpp_encode(TiktokenWrapper* wrapper, const char* text, size_t text_len, tiktoken_token_t** tokens_out);
extern void tiktoken_cpp_free(TiktokenWrapper* wrapper);

tiktoken_t tiktoken_get_encoding(const char* encoding_name) {
    return (tiktoken_t)tiktoken_cpp_get_encoding(encoding_name);
}

int tiktoken_encode(tiktoken_t encoding, const char* text, size_t text_len, tiktoken_token_t** tokens_out) {
    if (encoding == NULL || text == NULL || tokens_out == NULL) {
        return -1;
    }
    
    return tiktoken_cpp_encode((TiktokenWrapper*)encoding, text, text_len, tokens_out);
}

void tiktoken_free(tiktoken_t encoding) {
    if (encoding != NULL) {
        tiktoken_cpp_free((TiktokenWrapper*)encoding);
    }
}
