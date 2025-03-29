// Use a different include strategy
#include "../deps/tiktoken/encoding.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>

extern "C" {
#include "tiktoken.h"

// This struct holds the C++ encoder object
struct TiktokenWrapper {
    std::shared_ptr<tiktoken::GptEncoding> encoder;
};

// Maps string encoding names to tiktoken's LanguageModel enum
static tiktoken::LanguageModel get_language_model(const char* encoding_name) {
    if (strcmp(encoding_name, "cl100k_base") == 0) {
        return tiktoken::LanguageModel::CL100K_BASE;
    } else if (strcmp(encoding_name, "p50k_base") == 0) {
        return tiktoken::LanguageModel::P50K_BASE;
    } else if (strcmp(encoding_name, "r50k_base") == 0) {
        return tiktoken::LanguageModel::R50K_BASE;
    } else if (strcmp(encoding_name, "p50k_edit") == 0) {
        return tiktoken::LanguageModel::P50K_EDIT;
    } else if (strcmp(encoding_name, "o200k_base") == 0) {
        return tiktoken::LanguageModel::O200K_BASE;
    }
    // Default to CL100K_BASE if unknown
    return tiktoken::LanguageModel::CL100K_BASE;
}

TiktokenWrapper* tiktoken_cpp_get_encoding(const char* encoding_name) {
    try {
        TiktokenWrapper* wrapper = new TiktokenWrapper();
        wrapper->encoder = tiktoken::GptEncoding::get_encoding(get_language_model(encoding_name));
        return wrapper;
    } catch (const std::exception& e) {
        // Handle any exceptions from the C++ library
        return nullptr;
    }
}

int tiktoken_cpp_encode(TiktokenWrapper* wrapper, const char* text, size_t text_len, tiktoken_token_t** tokens_out) {
    try {
        if (wrapper == nullptr || wrapper->encoder == nullptr) {
            return -1;
        }

        // Create a string view from the text
        std::string text_str;
        if (text_len > 0) {
            text_str.assign(text, text_len);
        }
        
        // Encode the text
        std::vector<int> tokens = wrapper->encoder->encode(text_str);
        
        // Allocate memory for the result
        *tokens_out = (tiktoken_token_t*)malloc(tokens.size() * sizeof(tiktoken_token_t));
        if (*tokens_out == nullptr) {
            return -1;
        }
        
        // Copy the tokens
        std::memcpy(*tokens_out, tokens.data(), tokens.size() * sizeof(tiktoken_token_t));
        
        return static_cast<int>(tokens.size());
    } catch (const std::exception& e) {
        // Handle any exceptions
        return -1;
    }
}

void tiktoken_cpp_free(TiktokenWrapper* wrapper) {
    if (wrapper != nullptr) {
        delete wrapper;
    }
}

} // extern "C"
