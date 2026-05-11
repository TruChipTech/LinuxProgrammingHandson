/*
 * elf_artifact.c — Binary for ELF Analysis Exercises
 *
 * This file creates a binary with interesting properties for analysis:
 *   - Multiple sections (.text, .data, .bss, .rodata)
 *   - Global and local symbols
 *   - Library dependencies
 *   - Hidden messages in strings
 *   - Custom section attribute
 *
 * Build: gcc -g elf_artifact.c -o elf_artifact
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================
 * Global Data — ends up in different ELF sections
 * ============================================================ */

/* .data section: initialized global variables */
int global_counter = 100;
double pi_approx = 3.14159265358979;
char global_message[] = "Visible in .data section";

/* .rodata section: read-only/constant data */
const char *secret_message = "FLAG{y0u_f0und_the_h1dd3n_str1ng}";
const int magic_number = 0xCAFEBABE;
const char *author = "Binary Archaeologist Training Program v2.0";

/* .bss section: uninitialized global variables */
int uninitialized_array[256];
char bss_buffer[1024];
static int static_counter;

/* Custom section */
__attribute__((section(".artifact_meta")))
const char artifact_info[] = "ARTIFACT: Created for ELF analysis training";

/* ============================================================
 * Functions — .text section
 * ============================================================ */

/* A function with intentionally interesting disassembly */
int compute_checksum(const char *data, size_t len) {
    int checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum ^= data[i];
        checksum = (checksum << 3) | (checksum >> 29);
        checksum += (int)i;
    }
    return checksum;
}

/* Static function — symbol visibility differs from global */
static int helper_function(int x) {
    return x * x + 2 * x + 1;
}

/* Function that uses math library — requires -lm at link time */
double compute_distance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

/* Function that uses dynamic memory */
char *create_encoded_message(const char *input) {
    size_t len = strlen(input);
    char *encoded = malloc(len + 1);
    if (!encoded) return NULL;
    
    for (size_t i = 0; i < len; i++) {
        encoded[i] = input[i] ^ 0x42;  /* Simple XOR encoding */
    }
    encoded[len] = '\0';
    return encoded;
}

/* Function with multiple local variables — interesting stack frame */
void demonstrate_stack_layout(void) {
    int local_int = 42;
    double local_double = 3.14;
    char local_buffer[64];
    int local_array[10];
    
    snprintf(local_buffer, sizeof(local_buffer),
             "Stack local: int=%d, double=%.2f", local_int, local_double);
    
    for (int i = 0; i < 10; i++) {
        local_array[i] = i * local_int;
    }
    
    printf("%s\n", local_buffer);
    printf("Array sum: %d\n", local_array[9]);
}

/* ============================================================
 * MAIN
 * ============================================================ */

int main(int argc, char *argv[]) {
    printf("=== ELF Artifact Program ===\n");
    printf("Author: %s\n", author);
    printf("Magic: 0x%X\n", magic_number);
    
    /* Use various data sections */
    global_counter++;
    static_counter = 10;
    memset(bss_buffer, 'A', 64);
    bss_buffer[64] = '\0';
    
    printf("Counter: %d\n", global_counter);
    printf("Pi: %.6f\n", pi_approx);
    printf("Message: %s\n", global_message);
    
    /* Compute checksum */
    int chk = compute_checksum(global_message, strlen(global_message));
    printf("Checksum: 0x%08X\n", chk);
    
    /* Helper function */
    printf("Helper(5): %d\n", helper_function(5));
    
    /* Distance calculation */
    double dist = compute_distance(0, 0, 3, 4);
    printf("Distance: %.2f\n", dist);
    
    /* Encoded message */
    char *enc = create_encoded_message("Hello ELF World!");
    if (enc) {
        printf("Encoded message created (XOR 0x42)\n");
        
        /* Decode it back */
        for (size_t i = 0; i < strlen("Hello ELF World!"); i++) {
            enc[i] ^= 0x42;
        }
        printf("Decoded: %s\n", enc);
        free(enc);
    }
    
    /* Stack demo */
    demonstrate_stack_layout();
    
    /* BSS usage */
    printf("BSS buffer: %s\n", bss_buffer);
    printf("Uninitialized[0]: %d\n", uninitialized_array[0]);
    
    printf("\n%s\n", secret_message);
    printf("=== Artifact Program Complete ===\n");
    
    return 0;
}
