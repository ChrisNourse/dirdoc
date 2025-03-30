#!/usr/bin/env python3
import json
import base64
import os
import sys

def main():
    input_dir = "src/tiktoken_data"
    enc_name = "cl100k_base"
    json_path = f"{input_dir}/{enc_name}_data.json"
    
    if not os.path.exists(json_path):
        print(f"Error: Data file {json_path} not found.")
        print("Run extract_tiktoken_data.py first to generate the required data.")
        return 1
    
    try:
        # Load the JSON data
        with open(json_path, "r") as f:
            data = json.load(f)
    except Exception as e:
        print(f"Error loading JSON data: {e}")
        return 1
        
    # Generate tiktoken_data.h
    header_path = "src/tiktoken_data.h"
    with open(header_path, "w") as f:
        f.write("// Auto-generated from OpenAI's tiktoken library\n")
        f.write("// DO NOT EDIT MANUALLY\n\n")
        f.write("#ifndef TIKTOKEN_DATA_H\n")
        f.write("#define TIKTOKEN_DATA_H\n\n")
        
        f.write("#include <stddef.h>\n")
        f.write("#include <stdint.h>\n\n")

        # Special tokens
        f.write(f"#define TIKTOKEN_NUM_SPECIAL_TOKENS {len(data['special_tokens'])}\n\n")
        f.write("typedef struct {\n")
        f.write("    const char* token_b64;  // Base64 encoded token\n")
        f.write("    int id;                 // Token ID\n")
        f.write("} tiktoken_special_token_t;\n\n")
        
        f.write("static const tiktoken_special_token_t tiktoken_special_tokens[TIKTOKEN_NUM_SPECIAL_TOKENS] = {\n")
        for b64token, token_id in data['special_tokens'].items():
            f.write(f"    {{\"{b64token}\", {token_id}}},\n")
        f.write("};\n\n")
        
        # Vocabulary
        f.write(f"#define TIKTOKEN_VOCAB_SIZE {len(data['vocab'])}\n\n")
        f.write("typedef struct {\n")
        f.write("    const char* token_b64;  // Base64 encoded token bytes\n")
        f.write("    int id;                 // Token ID\n")
        f.write("} tiktoken_vocab_entry_t;\n\n")
        
        f.write("static const tiktoken_vocab_entry_t tiktoken_vocab[TIKTOKEN_VOCAB_SIZE] = {\n")
        for b64token, token_id in data['vocab'].items():
            f.write(f"    {{\"{b64token}\", {token_id}}},\n")
        f.write("};\n\n")
        
        # BPE merges
        merge_count = len(data.get('merges', []))
        f.write(f"#define TIKTOKEN_NUM_MERGES {merge_count}\n\n")
        
        if merge_count > 0:
            f.write("typedef struct {\n")
            f.write("    const char* first_b64;  // Base64 encoded first piece\n")
            f.write("    const char* second_b64; // Base64 encoded second piece\n")
            f.write("    int rank;               // Merge rank (lower = higher priority)\n")
            f.write("} tiktoken_bpe_merge_t;\n\n")
            
            f.write("static const tiktoken_bpe_merge_t tiktoken_bpe_merges[TIKTOKEN_NUM_MERGES] = {\n")
            for first_b64, second_b64, rank in data['merges']:
                f.write(f"    {{\"{first_b64}\", \"{second_b64}\", {rank}}},\n")
            f.write("};\n\n")
        else:
            f.write("// No BPE merges available\n")
            f.write("typedef struct {\n")
            f.write("    const char* first_b64;\n")
            f.write("    const char* second_b64;\n")
            f.write("    int rank;\n")
            f.write("} tiktoken_bpe_merge_t;\n\n")
            
            f.write("// Empty array as placeholder\n")
            f.write("static const tiktoken_bpe_merge_t tiktoken_bpe_merges[1] = {\n")
            f.write("    {NULL, NULL, 0}\n")
            f.write("};\n\n")
        
        f.write("#endif /* TIKTOKEN_DATA_H */\n")
        
    print(f"Generated C header file at {header_path}")
    print("Conversion completed successfully")
    return 0

if __name__ == "__main__":
    sys.exit(main())
