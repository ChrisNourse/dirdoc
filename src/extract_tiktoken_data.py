#!/usr/bin/env python3
import tiktoken
import json
import os
import sys
import base64

def main():
    output_dir = "src/tiktoken_data"
    os.makedirs(output_dir, exist_ok=True)

    # Focus on cl100k_base as it's used for GPT-3.5/GPT-4
    enc_name = "cl100k_base"
    
    try:
        encoding = tiktoken.get_encoding(enc_name)
    except Exception as e:
        print(f"Error: Failed to load tiktoken encoding '{enc_name}': {e}")
        print("Make sure the tiktoken Python package is installed (pip install tiktoken)")
        return 1
        
    print(f"Successfully loaded '{enc_name}' encoding")
    
    # Extract special tokens
    special_tokens = {}
    for token, token_id in encoding._special_tokens.items():
        # Convert token bytes to base64 to safely represent binary data in C code
        token_bytes = token.encode('utf-8')
        token_b64 = base64.b64encode(token_bytes).decode('ascii')
        special_tokens[token_b64] = token_id
    
    # Extract regular vocabulary
    vocab = {}
    for token_id, token_bytes in enumerate(encoding._tokens):
        # Skip if it's a special token ID
        if token_id in encoding._special_tokens.values():
            continue
        # Convert token bytes to base64
        token_b64 = base64.b64encode(token_bytes).decode('ascii')
        vocab[token_b64] = token_id
    
    # Extract BPE merges
    merges = []
    for (first, second), rank in encoding._mergeable_ranks.items():
        # Convert merge parts to base64
        first_b64 = base64.b64encode(first.encode('utf-8')).decode('ascii')
        second_b64 = base64.b64encode(second.encode('utf-8')).decode('ascii')
        merges.append((first_b64, second_b64, rank))
   
    # Create data structure
    data = {
        "special_tokens": special_tokens,
        "vocab": vocab,
        "merges": merges
    }
    
    # Write to JSON file
    json_path = f"{output_dir}/{enc_name}_data.json"
    with open(json_path, "w") as f:
        json.dump(data, f)
    print(f"Wrote encoding data to {json_path}")
    
    # Generate test cases for validation
    test_samples = [
        "Hello world",
        "const char *message = \"Hello, world!\";",
        "This is a test of the tiktoken encoding system.",
        "GPT-4 uses the cl100k_base encoding.",
        "Hello 你好 नमस्ते こんにちは"
    ]
    
    test_path = f"{output_dir}/token_tests.txt"
    with open(test_path, "w") as f:
        for sample in test_samples:
            tokens = encoding.encode(sample)
            f.write(f"{sample}\n")
            f.write(f"Token count: {len(tokens)}\n")
            f.write(f"Tokens: {tokens}\n\n")
    
    print(f"Wrote test cases to {test_path}")
    print("Extraction completed successfully")
    return 0

if __name__ == "__main__":
    sys.exit(main())
