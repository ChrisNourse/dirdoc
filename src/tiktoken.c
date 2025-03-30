#include "tiktoken.h"
#include <stdlib.h>
#include <string.h>

// Define a C-compatible struct for wrapper
typedef struct TiktokenWrapper TiktokenWrapper;

// External functions implemented in tiktoken_cpp.cpp
extern TiktokenWrapper* tiktoken_cpp_get_encoding(const char* encoding_name);
extern int tiktoken_cpp_encode(TiktokenWrapper* wrapper, const char* text, size_t text_len, tiktoken_token_t** tokens_out);
extern int tiktoken_cpp_count(TiktokenWrapper* wrapper, const char* text, size_t text_len);
extern void tiktoken_cpp_free(TiktokenWrapper* wrapper);
extern void tiktoken_cleanup(void);
extern bool init_tiktoken(void);

// External init function from C++
extern bool tiktoken_cpp_init(void);

// Global initialization function
bool init_tiktoken(void) {
    return tiktoken_cpp_init();
}

tiktoken_t tiktoken_get_encoding(const char* encoding_name) {
    return (tiktoken_t)tiktoken_cpp_get_encoding(encoding_name);
}

int tiktoken_encode(tiktoken_t encoding, const char* text, size_t text_len, tiktoken_token_t** tokens_out) {
    if (encoding == NULL || text == NULL || tokens_out == NULL) {
        return -1;
    }
    
    return tiktoken_cpp_encode((TiktokenWrapper*)encoding, text, text_len, tokens_out);
}

int tiktoken_count(tiktoken_t encoding, const char* text, size_t text_len) {
    if (encoding == NULL || text == NULL) {
        return -1;
    }
    
    return tiktoken_cpp_count((TiktokenWrapper*)encoding, text, text_len);
}

void tiktoken_free(tiktoken_t encoding) {
    if (encoding != NULL) {
        tiktoken_cpp_free((TiktokenWrapper*)encoding);
    }
}

// Register a cleanup function to be called at program exit
static void __attribute__((constructor)) tiktoken_register_cleanup(void) {
    atexit(tiktoken_cleanup);
}
