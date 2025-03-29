#ifndef TIKTOKEN_H
#define TIKTOKEN_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* tiktoken_t;
typedef int tiktoken_token_t;

/**
 * Get a tiktoken encoding by name
 * 
 * @param encoding_name The name of the encoding (e.g., "cl100k_base" for GPT-4 encoding)
 * @return A tiktoken encoding object, or NULL if the encoding is not found
 */
tiktoken_t tiktoken_get_encoding(const char* encoding_name);

/**
 * Encode a string into tokens
 * 
 * @param encoding The tiktoken encoding to use
 * @param text The text to encode
 * @param text_len Length of the text
 * @param tokens_out Output parameter for the array of tokens (caller must free)
 * @return The number of tokens, or -1 on error
 */
int tiktoken_encode(tiktoken_t encoding, const char* text, size_t text_len, tiktoken_token_t** tokens_out);

/**
 * Free a tiktoken encoding
 * 
 * @param encoding The tiktoken encoding to free
 */
void tiktoken_free(tiktoken_t encoding);

#ifdef __cplusplus
}
#endif

#endif /* TIKTOKEN_H */
