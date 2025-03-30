#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <map>

// Include base64 functions from the main project source
// Adjust path if necessary based on where this tool is compiled from
#include "../src/base64.h"

// Define the known special tokens for cl100k_base and their IDs
// We need their base64 representation to find them in the input file
const std::map<std::string, int> cl100k_special_tokens_b64 = {
    {base64_encode("<|endoftext|>"), 100257},
    {base64_encode("<|fim_prefix|>"), 100258},
    {base64_encode("<|fim_middle|>"), 100259},
    {base64_encode("<|fim_suffix|>"), 100260},
    {base64_encode("<|endofprompt|>"), 100276}
    // Add others here if needed for different encoders
};

struct VocabEntry {
    std::string token_b64;
    int id;
};

// Function to escape backslashes and quotes for C string literals
std::string escape_c_string(const std::string& s) {
    std::string escaped;
    for (char c : s) {
        if (c == '\\' || c == '"') {
            escaped += '\\';
        }
        escaped += c;
    }
    return escaped;
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_tiktoken_file> <output_header_file> <encoder_name>" << std::endl;
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];
    std::string encoder_name = argv[3]; // Currently unused, but kept for consistency

    std::ifstream input_file(input_path);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open input file: " << input_path << std::endl;
        return 1;
    }

    std::vector<VocabEntry> special_tokens;
    std::vector<VocabEntry> vocabulary;
    std::string line;
    int line_num = 0;

    // Skip the first line (version)
    if (!std::getline(input_file, line)) {
         std::cerr << "Error: Input file is empty or could not read version line: " << input_path << std::endl;
         return 1;
    }
    line_num++;

    // Read token data lines
    while (std::getline(input_file, line)) {
        line_num++;
        std::stringstream ss(line);
        std::string segment;
        std::vector<std::string> parts;

        // Split line by space
        while (std::getline(ss, segment, ' ')) {
            parts.push_back(segment);
        }

        if (parts.size() != 2) {
            std::cerr << "Warning: Skipping malformed line " << line_num << " in " << input_path << ": " << line << std::endl;
            continue;
        }

        std::string token_b64 = parts[0];
        int id;
        try {
            id = std::stoi(parts[1]);
        } catch (const std::invalid_argument& e) {
            std::cerr << "Warning: Invalid ID on line " << line_num << " in " << input_path << ": " << parts[1] << std::endl;
            continue;
        } catch (const std::out_of_range& e) {
             std::cerr << "Warning: ID out of range on line " << line_num << " in " << input_path << ": " << parts[1] << std::endl;
            continue;
        }

        // Check if this is a known special token
        auto special_it = cl100k_special_tokens_b64.find(token_b64);
        if (special_it != cl100k_special_tokens_b64.end()) {
            // Verify the ID matches the expected ID for the special token
            if (special_it->second == id) {
                special_tokens.push_back({token_b64, id});
            } else {
                 std::cerr << "Warning: Special token '" << base64_decode(token_b64) << "' (b64: " << token_b64
                           << ") found with unexpected ID " << id << " (expected " << special_it->second
                           << ") on line " << line_num << ". Treating as regular token." << std::endl;
                 // Add to regular vocabulary if ID doesn't match expectation
                 vocabulary.push_back({token_b64, id});
            }
        } else {
            // Regular vocabulary entry
            vocabulary.push_back({token_b64, id});
        }
    }

    input_file.close();

    // Sort entries by ID for consistency (optional, but matches Python script)
    std::sort(special_tokens.begin(), special_tokens.end(), [](const VocabEntry& a, const VocabEntry& b) {
        return a.id < b.id;
    });
    std::sort(vocabulary.begin(), vocabulary.end(), [](const VocabEntry& a, const VocabEntry& b) {
        return a.id < b.id;
    });


    // Generate the C header file
    std::ofstream output_file(output_path);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file for writing: " << output_path << std::endl;
        return 1;
    }

    std::cout << "Generating " << output_path << " for encoder '" << encoder_name << "'..." << std::endl;
    std::cout << "Found " << special_tokens.size() << " special tokens." << std::endl;
    std::cout << "Found " << vocabulary.size() << " regular vocabulary entries." << std::endl;

    output_file << "/*\n";
    output_file << " * Generated by tools/generate_tiktoken_data\n";
    output_file << " * Source: " << input_path << "\n";
    output_file << " * Encoder: " << encoder_name << "\n";
    output_file << " * DO NOT EDIT MANUALLY!\n";
    output_file << " */\n\n";
    output_file << "#ifndef TIKTOKEN_DATA_H\n";
    output_file << "#define TIKTOKEN_DATA_H\n\n";
    output_file << "#include <stddef.h> // For size_t\n\n";

    // --- Struct Definitions ---
    output_file << "// Structure for special tokens\n";
    output_file << "typedef struct {\n";
    output_file << "    const char* token_b64;  // Base64 encoded token bytes\n";
    output_file << "    int id;                 // Token ID\n";
    output_file << "} tiktoken_special_token_t;\n\n";

    output_file << "// Structure for vocabulary entries\n";
    output_file << "typedef struct {\n";
    output_file << "    const char* token_b64;  // Base64 encoded token bytes\n";
    output_file << "    int id;                 // Token ID\n";
    output_file << "} tiktoken_vocab_entry_t;\n\n";

    output_file << "// Structure for BPE merges (rank determines priority)\n";
    output_file << "typedef struct {\n";
    output_file << "    const char* first_b64;  // Base64 encoded first part of the pair\n";
    output_file << "    const char* second_b64; // Base64 encoded second part of the pair\n";
    output_file << "    int rank;               // Merge rank (lower is higher priority)\n";
    output_file << "} tiktoken_bpe_merge_t;\n\n";

    // --- Special Tokens ---
    output_file << "// Special Tokens\n";
    output_file << "static const tiktoken_special_token_t tiktoken_special_tokens[] = {\n";
    for (const auto& entry : special_tokens) {
        output_file << "    {\"" << escape_c_string(entry.token_b64) << "\", " << entry.id << "},\n";
    }
    output_file << "};\n";
    output_file << "static const size_t TIKTOKEN_NUM_SPECIAL_TOKENS = " << special_tokens.size() << ";\n\n";

    // --- Vocabulary ---
    output_file << "// Vocabulary (Token Bytes -> ID)\n";
    output_file << "static const tiktoken_vocab_entry_t tiktoken_vocab[] = {\n";
     for (const auto& entry : vocabulary) {
        output_file << "    {\"" << escape_c_string(entry.token_b64) << "\", " << entry.id << "},\n";
    }
    output_file << "};\n";
    output_file << "static const size_t TIKTOKEN_VOCAB_SIZE = " << vocabulary.size() << ";\n\n";

    // --- BPE Merges (Empty, matching Python script behavior) ---
    output_file << "// BPE Merges (First Bytes, Second Bytes -> Rank)\n";
    output_file << "// NOTE: Extraction of exact merge pairs from tiktoken lib is non-trivial.\n";
    output_file << "// The C++ code includes a fallback if this list is empty.\n";
    output_file << "static const tiktoken_bpe_merge_t tiktoken_bpe_merges[] = {\n";
    output_file << "    // { Base64(first_bytes), Base64(second_bytes), rank }\n";
    output_file << "};\n";
    output_file << "static const size_t TIKTOKEN_NUM_MERGES = 0;\n\n"; // Set count to 0

    output_file << "#endif // TIKTOKEN_DATA_H\n";

    output_file.close();

    std::cout << "Successfully wrote " << output_path << std::endl;

    return 0;
}
