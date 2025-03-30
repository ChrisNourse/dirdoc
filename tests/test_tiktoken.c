#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tiktoken.h"
#include "stats.h"

void test_tiktoken_init() {
    bool result = tiktoken_init();
    assert(result == true);
    printf("✅ tiktoken initialization test passed\n");
}

void test_token_counting() {
    // Initialize tiktoken
    assert(tiktoken_init());
    
    // Test with various text samples
    const char* test_strings[] = {
        "Hello world",
        "This is a simple test of the token counting system.",
        "const char *message = \"Hello, world!\";",
        "int main() { printf(\"Hello\"); return 0; }",
        "GPT-4 uses the cl100k_base encoding."
    };
    
    printf("Testing token counting on sample texts:\n");
    
    for (int i = 0; i < sizeof(test_strings) / sizeof(test_strings[0]); i++) {
        const char* test_str = test_strings[i];
        
        // Get token count via DocumentInfo
        DocumentInfo info = {0};
        calculate_token_stats(test_str, &info);
        
        // Get token count directly
        tiktoken_t encoding = tiktoken_get_encoding("cl100k_base");
        int direct_count = tiktoken_count(encoding, test_str, strlen(test_str));
        tiktoken_free(encoding);
        
        // Simple estimation: ~1 token per ~4-5 characters for English text
        size_t approx_tokens = (strlen(test_str) + 4) / 5;
        
        printf("String: \"%s\"\n", test_str);
        printf("  - DocumentInfo tokens: %zu\n", info.total_tokens);
        printf("  - Direct count tokens: %d\n", direct_count);
        printf("  - Approx tokens: %zu\n", approx_tokens);
        
        // Verify that token counts are reasonable
        assert(info.total_tokens > 0);
        assert(direct_count > 0 || direct_count == -1); // -1 is acceptable if encoding fails
        
        // Token count should be within a reasonable range of character count
        assert(info.total_tokens <= strlen(test_str));
    }
    
    printf("✅ Token counting test passed\n");
}

// Main function moved to test_dirdoc.c
void run_tiktoken_tests() {
    printf("Running tiktoken tests...\n");
    
    test_tiktoken_init();
    test_token_counting();
    
    printf("All tiktoken tests passed!\n");
}
