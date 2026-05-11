/*
 * macro_magic.c — Preprocessor Deep Dive
 *
 * Demonstrates:
 *   - Conditional compilation with #ifdef / #elif / #else
 *   - Platform-specific macros
 *   - Macro tricks: stringification, token pasting, variadic macros
 *   - Include guards pattern
 *
 * Compile:
 *   gcc -DPLATFORM_X86 macro_magic.c -o macro_x86
 *   gcc -DPLATFORM_ARM macro_magic.c -o macro_arm
 *   gcc -E macro_magic.c -o macro_magic.i   (to see expansion)
 */

#include <stdio.h>
#include <stdint.h>

/* ============================================================
 * PLATFORM ABSTRACTION via Preprocessor
 * ============================================================ */

#if defined(PLATFORM_ARM)
    #define ARCH_NAME       "ARM"
    #define PAGE_SIZE       4096
    #define CACHE_LINE_SIZE 64
    #define ENDIANNESS      "Little Endian"
#elif defined(PLATFORM_X86)
    #define ARCH_NAME       "x86_64"
    #define PAGE_SIZE       4096
    #define CACHE_LINE_SIZE 64
    #define ENDIANNESS      "Little Endian"
/* TODO: Exercise — Add PLATFORM_RISCV here
 * #elif defined(PLATFORM_RISCV)
 *     #define ARCH_NAME       "RISC-V"
 *     #define PAGE_SIZE       ???
 *     #define CACHE_LINE_SIZE ???
 *     #define ENDIANNESS      ???
 */
#else
    #define ARCH_NAME       "Unknown"
    #define PAGE_SIZE       4096
    #define CACHE_LINE_SIZE 64
    #define ENDIANNESS      "Unknown"
    #warning "No platform defined! Use -DPLATFORM_X86 or -DPLATFORM_ARM"
#endif

/* ============================================================
 * MACRO TRICKS
 * ============================================================ */

/* Stringification: convert macro argument to string literal */
#define STR(x)          #x
#define XSTR(x)         STR(x)

/* Token pasting: concatenate tokens */
#define REGISTER(n)     reg_##n
#define DECLARE_REG(n)  int REGISTER(n) = n

/* Variadic macros */
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

/* Compile-time assertion (pre C11 trick) */
#define STATIC_ASSERT(cond, msg) \
    typedef char static_assert_##msg[(cond) ? 1 : -1]

/* Min/Max with type safety warning */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Array size macro */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Bit manipulation macros */
#define BIT_SET(val, bit)    ((val) | (1U << (bit)))
#define BIT_CLEAR(val, bit)  ((val) & ~(1U << (bit)))
#define BIT_TOGGLE(val, bit) ((val) ^ (1U << (bit)))
#define BIT_CHECK(val, bit)  (((val) >> (bit)) & 1U)

/* ============================================================
 * COMPILE-TIME CHECKS
 * ============================================================ */

STATIC_ASSERT(sizeof(int) >= 4, int_must_be_at_least_4_bytes);
STATIC_ASSERT(PAGE_SIZE == 4096, page_size_must_be_4096);

/* ============================================================
 * FUNCTIONS
 * ============================================================ */

void print_platform_info(void) {
    printf("╔══════════════════════════════════╗\n");
    printf("║     Platform Configuration       ║\n");
    printf("╠══════════════════════════════════╣\n");
    printf("║ Architecture: %-18s ║\n", ARCH_NAME);
    printf("║ Page Size:    %-18d ║\n", PAGE_SIZE);
    printf("║ Cache Line:   %-18d ║\n", CACHE_LINE_SIZE);
    printf("║ Endianness:   %-18s ║\n", ENDIANNESS);
    printf("╚══════════════════════════════════╝\n");
}

void demonstrate_macros(void) {
    /* Token pasting */
    DECLARE_REG(0);
    DECLARE_REG(1);
    DECLARE_REG(7);
    
    printf("\nRegister values: r0=%d, r1=%d, r7=%d\n",
           REGISTER(0), REGISTER(1), REGISTER(7));
    
    /* Stringification */
    printf("PAGE_SIZE macro expands to: %s = %d\n",
           XSTR(PAGE_SIZE), PAGE_SIZE);
    
    /* Variadic debug printing */
    DEBUG_PRINT("Starting macro demo");
    DEBUG_PRINT("Platform: %s, Page Size: %d", ARCH_NAME, PAGE_SIZE);
    
    /* Min/Max */
    int a = 42, b = 17;
    printf("MIN(%d, %d) = %d\n", a, b, MIN(a, b));
    printf("MAX(%d, %d) = %d\n", a, b, MAX(a, b));
    
    /* Array size */
    int arr[] = {10, 20, 30, 40, 50};
    printf("Array has %zu elements\n", ARRAY_SIZE(arr));
    
    /* Bit manipulation */
    uint32_t flags = 0;
    flags = BIT_SET(flags, 3);     /* Set bit 3 */
    flags = BIT_SET(flags, 7);     /* Set bit 7 */
    printf("Flags after setting bits 3,7: 0x%08X\n", flags);
    printf("Bit 3 is: %u\n", BIT_CHECK(flags, 3));
    printf("Bit 4 is: %u\n", BIT_CHECK(flags, 4));
    flags = BIT_TOGGLE(flags, 3);
    printf("Flags after toggling bit 3: 0x%08X\n", flags);
}

int main(void) {
    printf("=== Macro Magic Demonstration ===\n\n");
    
    print_platform_info();
    demonstrate_macros();
    
    printf("\n=== Built with GCC %d.%d.%d ===\n",
           __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    
    return 0;
}
