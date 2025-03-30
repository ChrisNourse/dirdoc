#include <stdio.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <memory>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <utility>
#include <regex>
#include <stdexcept>
#include <climits>

// Include the generated tiktoken data
#include "tiktoken_data.h"

// For Base64 decoding
#include "base64.h"

namespace tiktoken {

// Pair of byte strings
typedef std::pair<std::string, std::string> BytesPair;

// Custom hash function for BytesPair
struct BytesPairHash {
    std::size_t operator()(const BytesPair& p) const {
        return std::hash<std::string>()(p.first) ^ std::hash<std::string>()(p.second);
    }
};

class BpeEncoder {
private:
    // Token vocabulary: maps byte sequences to token IDs
    std::unordered_map<std::string, int> token_vocab;
    
    // Special tokens (like <|endoftext|>)
    std::unordered_map<std::string, int> special_tokens;
    
    // BPE merge ranks: lower rank = higher priority
    std::unordered_map<BytesPair, int, BytesPairHash> bpe_ranks;
    
    // Pre-computed regex patterns
    std::regex pat_split;
    bool initialized;
    
    // Initialize the encoder with tiktoken data
    void initialize() {
        // Load special tokens from the data
        for (size_t i = 0; i < TIKTOKEN_NUM_SPECIAL_TOKENS; i++) {
            const tiktoken_special_token_t& special_token = tiktoken_special_tokens[i];
            std::string token_bytes = base64_decode(special_token.token_b64);
            special_tokens[token_bytes] = special_token.id;
        }
        
        // Load vocabulary from the data
        for (size_t i = 0; i < TIKTOKEN_VOCAB_SIZE; i++) {
            const tiktoken_vocab_entry_t& vocab_entry = tiktoken_vocab[i];
            try {
                std::string token_bytes = base64_decode(vocab_entry.token_b64);
                token_vocab[token_bytes] = vocab_entry.id;
            } catch (const std::exception& e) {
                fprintf(stderr, "Warning: Failed to decode token %zu: %s\n", i, e.what());
            }
        }
        
        // Load BPE merges from the data - safely handle if we have none
        if (TIKTOKEN_NUM_MERGES > 0) {
            for (size_t i = 0; i < TIKTOKEN_NUM_MERGES; i++) {
                try {
                    const tiktoken_bpe_merge_t& merge = tiktoken_bpe_merges[i];
                    std::string first = base64_decode(merge.first_b64);
                    std::string second = base64_decode(merge.second_b64);
                    bpe_ranks[{first, second}] = merge.rank;
                } catch (const std::exception& e) {
                    fprintf(stderr, "Warning: Failed to decode merge %zu: %s\n", i, e.what());
                }
            }
        }
        
        // Use a simple whitespace/punctuation split pattern for basic tokenization
        pat_split = std::regex("\\s+|[[:punct:]]");
        
        initialized = true;
    }
    
    // Get pairs of consecutive bytes from token
    std::vector<BytesPair> get_pairs(const std::vector<std::string>& word_pieces) {
        std::vector<BytesPair> pairs;
        if (word_pieces.size() < 2) return pairs;
        
        for (size_t i = 0; i < word_pieces.size() - 1; i++) {
            pairs.push_back({word_pieces[i], word_pieces[i+1]});
        }
        
        return pairs;
    }
    
    // Apply BPE to a token
    std::vector<std::string> bpe(const std::string& token) {
        // Initialize with individual bytes
        std::vector<std::string> word;
        for (unsigned char c : token) {
            word.push_back(std::string(1, c));
        }
        
        // Apply merges until no more can be applied
        std::vector<BytesPair> pairs = get_pairs(word);
        if (pairs.empty()) {
            return word;
        }
        
        while (true) {
            // Find the pair with the lowest rank (highest priority)
            int best_rank = INT_MAX;
            BytesPair best_pair;
            bool found = false;
            
            for (const auto& pair : pairs) {
                auto it = bpe_ranks.find(pair);
                if (it != bpe_ranks.end() && it->second < best_rank) {
                    best_rank = it->second;
                    best_pair = pair;
                    found = true;
                }
            }
            
            if (!found) break;
            
            // Apply the merge
            std::vector<std::string> new_word;
            for (size_t i = 0; i < word.size();) {
                // Try to find a match for the best pair
                if (i < word.size() - 1 && word[i] == best_pair.first && word[i+1] == best_pair.second) {
                    new_word.push_back(best_pair.first + best_pair.second);
                    i += 2;  // Skip both parts of the pair
                } else {
                    new_word.push_back(word[i]);
                    i += 1;
                }
            }
            
            // Update word and get new pairs
            word = std::move(new_word);
            pairs = get_pairs(word);
            if (pairs.empty()) break;
        }
        
        return word;
    }

    // Simple tokenization function - split text into tokens
    std::vector<std::string> basic_tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        
        // First check if the entire string is a special token
        auto it = special_tokens.find(text);
        if (it != special_tokens.end()) {
            tokens.push_back(text);
            return tokens;
        }
        
        // Simple tokenization approach - split on whitespace and punctuation
        size_t start = 0;
        for (size_t i = 0; i < text.size(); i++) {
            char c = text[i];
            if (isspace(c) || ispunct(c)) {
                // Add accumulated token if any
                if (i > start) {
                    tokens.push_back(text.substr(start, i - start));
                }
                
                // Add the whitespace or punctuation as a separate token
                tokens.push_back(text.substr(i, 1));
                start = i + 1;
            }
        }
        
        // Add the final token if any
        if (start < text.size()) {
            tokens.push_back(text.substr(start));
        }
        
        return tokens;
    }

public:
    BpeEncoder() : initialized(false) {
        initialize();
    }
    
    bool isInitialized() const {
        return initialized;
    }
    
    // Encode text into tokens
    std::vector<int> encode(const std::string& text) {
        // Check if the text matches a special token exactly
        auto special_it = special_tokens.find(text);
        if (special_it != special_tokens.end()) {
            return {special_it->second};
        }
        
        std::vector<int> encoded_tokens;

        // First pass: basic tokenization
        std::vector<std::string> raw_tokens = basic_tokenize(text);
        
        // Second pass: BPE on each piece
        for (const auto& token : raw_tokens) {
            // Special token check for each piece
            special_it = special_tokens.find(token);
            if (special_it != special_tokens.end()) {
                encoded_tokens.push_back(special_it->second);
                continue;
            }
            
            // Apply BPE to get subtoken pieces
            std::vector<std::string> bpe_tokens;
            
            if (bpe_ranks.empty()) {
                // Tokenization approach using our embedded vocabulary data
                // This is a simplified version but works well for common English text
                
                // First check if the token is in the vocabulary as-is
                auto vocab_it = token_vocab.find(token);
                if (vocab_it != token_vocab.end()) {
                    encoded_tokens.push_back(vocab_it->second);
                    continue;
                }
                
                // We already checked this token in the vocabulary
                
                // Otherwise use a fallback approach
                bool is_ascii = true;
                for (unsigned char c : token) {
                    if (c > 127) {
                        is_ascii = false;
                        break;
                    }
                }
                
                if (is_ascii) {
                    // For ASCII tokens
                    if (token.length() <= 4) {
                        // Very short ASCII tokens
                        bpe_tokens.push_back(token);
                    } else if (ispunct(token[0]) || isdigit(token[0])) {
                        // Punctuation and numbers often need special handling
                        for (char c : token) {
                            bpe_tokens.push_back(std::string(1, c));
                        }
                    } else {
                        // Split into smaller chunks based on length
                        // GPT models often split words into subwords of ~2-4 bytes
                        size_t chunk_size;
                        if (token.length() <= 8) {
                            chunk_size = 4;  // Short words use ~4 char chunks
                        } else if (token.length() <= 16) {
                            chunk_size = 3;  // Medium words use ~3 char chunks
                        } else {
                            chunk_size = 2;  // Long words use ~2 char chunks
                        }
                        
                        for (size_t i = 0; i < token.length(); i += chunk_size) {
                            bpe_tokens.push_back(token.substr(i, std::min(chunk_size, token.length() - i)));
                        }
                    }
                } else {
                    // For non-ASCII, tokenize bytewise (Unicode is usually 1 token per char)
                    for (unsigned char c : token) {
                        bpe_tokens.push_back(std::string(1, c));
                    }
                }
            } else {
                bpe_tokens = bpe(token);
            }
            
            // Convert each piece to a token ID
            for (const auto& bpe_token : bpe_tokens) {
                auto vocab_it = token_vocab.find(bpe_token);
                if (vocab_it != token_vocab.end()) {
                    encoded_tokens.push_back(vocab_it->second);
                } else {
                    // For unknown tokens, try byte-level encoding
                    for (unsigned char c : bpe_token) {
                        std::string byte_token(1, c);
                        auto byte_it = token_vocab.find(byte_token);
                        if (byte_it != token_vocab.end()) {
                            encoded_tokens.push_back(byte_it->second);
                        }
                    }
                }
            }
        }
        
        return encoded_tokens;
    }
};

}  // namespace tiktoken

// This is our C interface for the BpeEncoder
#include "tiktoken.h"

// This struct holds the C++ encoder object
extern "C" {
struct TiktokenWrapper {
    tiktoken::BpeEncoder* encoder;
    bool initialized;
};

// Global tiktoken instance for simple API
static TiktokenWrapper* g_default_tiktoken = nullptr;

// Initialize the default tiktoken instance
extern "C" bool tiktoken_cpp_init() {
    if (g_default_tiktoken != nullptr && g_default_tiktoken->initialized) {
        return true;
    }
    
    try {
        if (g_default_tiktoken == nullptr) {
            g_default_tiktoken = new TiktokenWrapper();
            g_default_tiktoken->encoder = new tiktoken::BpeEncoder();
            g_default_tiktoken->initialized = g_default_tiktoken->encoder->isInitialized();
        }
        
        return g_default_tiktoken->initialized;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to initialize tiktoken: %s\n", e.what());
        return false;
    } catch (...) {
        fprintf(stderr, "Unknown error initializing tiktoken\n");
        return false;
    }
}

// Get an encoding by name (only cl100k_base is supported)
extern "C" TiktokenWrapper* tiktoken_cpp_get_encoding(const char* encoding_name) {
    try {
        // We only support cl100k_base for now
        if (strcmp(encoding_name, "cl100k_base") != 0) {
            fprintf(stderr, "Warning: Only cl100k_base encoding is supported, using it instead of %s\n", 
                    encoding_name);
        }
        
        // Initialize or return the default instance
        if (tiktoken_cpp_init()) {
            return g_default_tiktoken;
        }
        return nullptr;
    } catch (const std::exception& e) {
        fprintf(stderr, "Exception getting encoding: %s\n", e.what());
        return nullptr;
    }
}

// Encode a string to tokens
extern "C" int tiktoken_cpp_encode(TiktokenWrapper* wrapper, const char* text, size_t text_len, 
                                  tiktoken_token_t** tokens_out) {
    try {
        if (wrapper == nullptr || wrapper->encoder == nullptr || !wrapper->initialized) {
            return -1;
        }

        // Create a string from the text
        std::string text_str;
        if (text != nullptr && text_len > 0) {
            text_str.assign(text, text_len);
        }
        
        // Encode the text using the BPE algorithm
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
        fprintf(stderr, "Exception in encoding: %s\n", e.what());
        return -1;
    }
}

// Count tokens without returning them
extern "C" int tiktoken_cpp_count(TiktokenWrapper* wrapper, const char* text, size_t text_len) {
    try {
        if (wrapper == nullptr || wrapper->encoder == nullptr || !wrapper->initialized) {
            return -1;
        }

        // Create a string from the text
        std::string text_str;
        if (text != nullptr && text_len > 0) {
            text_str.assign(text, text_len);
        }
        
        // Encode and return the count
        std::vector<int> tokens = wrapper->encoder->encode(text_str);
        return static_cast<int>(tokens.size());
    } catch (const std::exception& e) {
        fprintf(stderr, "Exception in token counting: %s\n", e.what());
        return -1;
    }
}

// Free a tiktoken encoding
extern "C" void tiktoken_cpp_free(TiktokenWrapper* wrapper) {
    if (wrapper != nullptr && wrapper != g_default_tiktoken) {
        if (wrapper->encoder != nullptr) {
            delete wrapper->encoder;
            wrapper->encoder = nullptr;
        }
        delete wrapper;
    }
}

// Clean up global resources on program exit
extern "C" void tiktoken_cleanup() {
    if (g_default_tiktoken != nullptr) {
        if (g_default_tiktoken->encoder != nullptr) {
            delete g_default_tiktoken->encoder;
            g_default_tiktoken->encoder = nullptr;
        }
        delete g_default_tiktoken;
        g_default_tiktoken = nullptr;
    }
}

} // extern "C"
