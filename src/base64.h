#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>
#include <stdexcept>

// Base64 decoding function
inline std::string base64_decode(const std::string& base64_str) {
    const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
        
    std::string output;
    std::vector<int> T(256, -1);
    
    // Initialize lookup table
    for (int i = 0; i < 64; i++) {
        T[base64_chars[i]] = i;
    }
    
    // Process the input in groups of 4 characters
    for (size_t i = 0; i < base64_str.size(); i += 4) {
        int n = 0;
        int valid_chars = 0;
        
        for (int j = 0; j < 4 && i + j < base64_str.size(); ++j) {
            char c = base64_str[i + j];
            if (c == '=') {
                break;  // Padding character
            }
            
            int value = T[c];
            if (value == -1) {
                continue;  // Skip invalid characters
            }
            
            n = (n << 6) | value;
            valid_chars++;
        }
        
        // No valid base64 characters found in this group
        if (valid_chars == 0) {
            continue;
        }
        
        // Extract bytes based on how many valid characters we saw
        if (valid_chars >= 2) {
            output += static_cast<char>((n >> 4) & 0xFF);
        }
        if (valid_chars >= 3) {
            output += static_cast<char>((n >> 2) & 0xFF); 
        }
        if (valid_chars >= 4) {
            output += static_cast<char>(n & 0xFF);
        }
    }
    
    return output;
}

#endif // BASE64_H
