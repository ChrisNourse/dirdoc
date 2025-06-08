#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>
#include <stdexcept>

/**
 * @brief Decode a base64-encoded string.
 *
 * @param base64_str Input base64 string.
 * @return Decoded binary data as a string.
 */
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


/**
 * @brief Encode binary data as base64.
 *
 * @param input Raw input string.
 * @return Base64 encoded representation.
 */
inline std::string base64_encode(const std::string& input) {
    const char* base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string output;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    size_t input_len = input.length();
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(input.c_str());

    while (input_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i < 4) ; i++)
                output += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            output += base64_chars[char_array_4[j]];

        while((i++ < 3))
            output += '=';
    }

    return output;
}


#endif // BASE64_H
