/*
 * main_caller.c — C program that calls assembly functions
 *
 * Demonstrates cross-language interoperability between C and x86_64 assembly.
 *
 * Build:
 *   nasm -f elf64 asm_add.s -o asm_add.o
 *   gcc -c main_caller.c -o main_caller.o
 *   gcc main_caller.o asm_add.o -o mixed_lang
 */

#include <stdio.h>

/* Declare the external assembly functions */
extern int asm_add(int a, int b);

/* TODO: Uncomment when you implement asm_multiply in asm_add.s */
/* extern int asm_multiply(int a, int b); */

int main(void) {
    int a = 25;
    int b = 17;
    
    printf("=== C ↔ Assembly Interoperability Demo ===\n\n");
    
    /* Call assembly function from C */
    int sum = asm_add(a, b);
    printf("asm_add(%d, %d) = %d\n", a, b, sum);
    
    /* Verify correctness */
    int expected = a + b;
    if (sum == expected) {
        printf("✓ Result is CORRECT!\n");
    } else {
        printf("✗ Result is WRONG! Expected %d, got %d\n", expected, sum);
    }
    
    /* TODO: Uncomment when you implement asm_multiply
    int product = asm_multiply(a, b);
    printf("\nasm_multiply(%d, %d) = %d\n", a, b, product);
    
    int expected_prod = a * b;
    if (product == expected_prod) {
        printf("✓ Multiply result is CORRECT!\n");
    } else {
        printf("✗ Multiply result is WRONG! Expected %d, got %d\n",
               expected_prod, product);
    }
    */
    
    /* Test with multiple values */
    printf("\n--- Batch Test ---\n");
    int test_cases[][2] = {{1, 2}, {100, 200}, {-5, 10}, {0, 0}, {-30, -12}};
    int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    int passed = 0;
    
    for (int i = 0; i < num_tests; i++) {
        int result = asm_add(test_cases[i][0], test_cases[i][1]);
        int expect = test_cases[i][0] + test_cases[i][1];
        const char *status = (result == expect) ? "PASS" : "FAIL";
        if (result == expect) passed++;
        printf("  asm_add(%4d, %4d) = %4d  [%s]\n",
               test_cases[i][0], test_cases[i][1], result, status);
    }
    
    printf("\nResults: %d/%d tests passed\n", passed, num_tests);
    printf("=== Demo Complete ===\n");
    
    return 0;
}
