/*
 * mystery_binary.c — Binary Forensics Challenge
 *
 * INSTRUCTOR NOTE: Compile this and give students ONLY the binary (stripped).
 *   gcc -O2 mystery_binary.c -o mystery -lm
 *   strip mystery
 *
 * Students must analyze the binary WITHOUT seeing this source code.
 * The program performs several operations that leave distinctive traces
 * in strings, system calls, and library calls.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Hidden strings for students to find */
static const char *PROGRAM_ID = "MYSTERY-v3.7-alpha";
static const char *SECRET_KEY = "XK9-DELTA-FOXTROT-2026";
static const char *HIDDEN_URL = "config.internal.lab:8080/api/status";

/* Obfuscated function names to make disassembly interesting */
void phase_alpha(void) {
    /* File operations — detectable via strace */
    FILE *fp = fopen("/tmp/.mystery_cache", "w");
    if (fp) {
        fprintf(fp, "timestamp=%ld\n", (long)time(NULL));
        fprintf(fp, "pid=%d\n", getpid());
        fclose(fp);
    }
}

int phase_beta(const char *input) {
    /* String processing — detectable via ltrace */
    int hash = 0;
    size_t len = strlen(input);
    for (size_t i = 0; i < len; i++) {
        hash = hash * 31 + input[i];
    }
    return hash;
}

double phase_gamma(int iterations) {
    /* Math operations — shows libm dependency */
    double result = 0.0;
    for (int i = 1; i <= iterations; i++) {
        result += sin((double)i / 100.0) * cos((double)i / 200.0);
    }
    return result;
}

void phase_delta(void) {
    /* Environment inspection — detectable via strace */
    const char *user = getenv("USER");
    const char *home = getenv("HOME");
    const char *shell = getenv("SHELL");
    
    printf("Environment scan complete\n");
    if (user) {
        printf("  Operator: %s\n", user);
    }
    /* Intentionally doesn't print home/shell but accesses them */
    (void)home;
    (void)shell;
}

/* XOR encode/decode function — interesting in disassembly */
void xor_transform(char *data, size_t len, unsigned char key) {
    for (size_t i = 0; i < len; i++) {
        data[i] ^= key;
    }
}

int main(int argc, char *argv[]) {
    printf("[%s] Initializing...\n", PROGRAM_ID);
    
    /* Phase Alpha: File operations */
    phase_alpha();
    printf("[*] Phase Alpha complete\n");
    
    /* Phase Beta: String hashing */
    int hash = phase_beta(SECRET_KEY);
    printf("[*] Phase Beta: hash=0x%08X\n", hash);
    
    /* Phase Gamma: Math computation */
    double val = phase_gamma(10000);
    printf("[*] Phase Gamma: result=%.6f\n", val);
    
    /* Phase Delta: Environment */
    phase_delta();
    printf("[*] Phase Delta complete\n");
    
    /* XOR transformation */
    char message[] = "Operation successful";
    xor_transform(message, strlen(message), 0x55);
    /* message is now garbled */
    xor_transform(message, strlen(message), 0x55);
    /* message is restored */
    printf("[*] Final: %s\n", message);
    
    /* Cleanup */
    unlink("/tmp/.mystery_cache");
    
    printf("[%s] All phases complete. Status: NOMINAL\n", PROGRAM_ID);
    return 0;
}
