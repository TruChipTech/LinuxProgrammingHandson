/*
 * pipeline_demo.c — GCC Compilation Pipeline Demonstration
 * 
 * This file is designed to showcase each stage of the GCC compilation pipeline.
 * Compile each stage separately:
 *   gcc -E pipeline_demo.c -o pipeline_demo.i   (Preprocessing)
 *   gcc -S pipeline_demo.i -o pipeline_demo.s   (Compilation)
 *   gcc -c pipeline_demo.s -o pipeline_demo.o   (Assembly)
 *   gcc pipeline_demo.o -o pipeline_demo -lm    (Linking)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================
 * PREPROCESSOR DIRECTIVES — These are resolved in Stage 1 (-E)
 * ============================================================ */

#define APP_NAME    "Pipeline Demo"
#define VERSION     "1.0.0"
#define MAX_ITEMS   10

/* Stringification macro */
#define STRINGIFY(x)    #x
#define TOSTRING(x)     STRINGIFY(x)

/* Token pasting macro */
#define MAKE_FUNC(name) void func_##name(void) { \
    printf("Function: " #name "\n"); \
}

/* Conditional compilation */
#ifdef DEBUG
    #define LOG(msg) printf("[DEBUG] %s:%d: %s\n", __FILE__, __LINE__, msg)
#else
    #define LOG(msg) /* No-op in release */
#endif

/* Built-in macros */
#define SHOW_BUILD_INFO() do { \
    printf("Compiled on: %s at %s\n", __DATE__, __TIME__); \
    printf("File: %s, Line: %d\n", __FILE__, __LINE__); \
    printf("C Standard: %ld\n", __STDC_VERSION__); \
} while(0)

/* Generate functions using macro */
MAKE_FUNC(alpha)
MAKE_FUNC(beta)

/* ============================================================
 * GLOBAL DATA — These go into .data and .bss sections
 * ============================================================ */

int initialized_global = 42;           /* .data section */
int uninitialized_global;              /* .bss section */
const char *greeting = "Hello from the pipeline!";  /* .rodata */

/* ============================================================
 * FUNCTIONS — These go into .text section
 * ============================================================ */

/* A simple function to demonstrate call mechanics in assembly */
int add_numbers(int a, int b) {
    return a + b;
}

/* Function using math library — requires -lm at link stage */
double compute_hypotenuse(double a, double b) {
    return sqrt(a * a + b * b);
}

/* Function with local variables — see stack frame in assembly */
void demonstrate_stack(void) {
    int local_array[MAX_ITEMS];
    int i;
    
    for (i = 0; i < MAX_ITEMS; i++) {
        local_array[i] = i * i;
    }
    
    printf("Stack demo: array[5] = %d\n", local_array[5]);
}

/* ============================================================
 * MAIN — Entry point
 * ============================================================ */

int main(int argc, char *argv[]) {
    printf("=== %s v%s ===\n", APP_NAME, VERSION);
    
    SHOW_BUILD_INFO();
    LOG("This only appears in debug builds");
    
    /* Demonstrate function calls */
    int sum = add_numbers(10, 32);
    printf("Sum: %d\n", sum);
    
    double hyp = compute_hypotenuse(3.0, 4.0);
    printf("Hypotenuse: %.2f\n", hyp);
    
    /* Demonstrate macro-generated functions */
    func_alpha();
    func_beta();
    
    /* Demonstrate stack usage */
    demonstrate_stack();
    
    /* Show global variable usage */
    printf("Initialized global: %d\n", initialized_global);
    printf("Uninitialized global: %d\n", uninitialized_global);
    printf("Greeting: %s\n", greeting);
    
    printf("=== Pipeline Demo Complete ===\n");
    return 0;
}
