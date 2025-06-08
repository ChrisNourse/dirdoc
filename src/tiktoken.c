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

// External init function from C++
extern bool tiktoken_cpp_init(void);

/**
 * @brief Initialize the underlying tiktoken library.
 *
 * @return true on success, false otherwise.
 */
bool tiktoken_init(void) {
    return tiktoken_cpp_init();
}

/**
 * @brief Obtain a tiktoken encoding by name.
 *
 * @param encoding_name Encoding identifier, e.g. "cl100k_base".
 * @return tiktoken_t Encoding handle or NULL on error.
 */
tiktoken_t tiktoken_get_encoding(const char* encoding_name) {
    return (tiktoken_t)tiktoken_cpp_get_encoding(encoding_name);
}

/**
 * @brief Encode a string into tokens.
 *
 * @param encoding Encoding handle.
 * @param text Text to encode.
 * @param text_len Length of text.
 * @param tokens_out Output array of tokens (allocated with malloc()).
 * @return int Number of tokens or -1 on error.
 */
int tiktoken_encode(tiktoken_t encoding, const char* text, size_t text_len, tiktoken_token_t** tokens_out) {
    if (encoding == NULL || text == NULL || tokens_out == NULL) {
        return -1;
    }
    
    return tiktoken_cpp_encode((TiktokenWrapper*)encoding, text, text_len, tokens_out);
}

/**
 * @brief Count tokens in a string without returning them.
 *
 * @param encoding Encoding handle.
 * @param text Text to count.
 * @param text_len Length of text.
 * @return int Number of tokens or -1 on error.
 */
int tiktoken_count(tiktoken_t encoding, const char* text, size_t text_len) {
    if (encoding == NULL || text == NULL) {
        return -1;
    }
    
    return tiktoken_cpp_count((TiktokenWrapper*)encoding, text, text_len);
}

/**
 * @brief Free an encoding instance.
 *
 * @param encoding Encoding handle to free.
 */
void tiktoken_free(tiktoken_t encoding) {
    if (encoding != NULL) {
        tiktoken_cpp_free((TiktokenWrapper*)encoding);
    }
}

/**
 * @brief Register cleanup of global resources at process exit.
 */
static void __attribute__((constructor)) tiktoken_register_cleanup(void) {
    atexit(tiktoken_cleanup);
}
