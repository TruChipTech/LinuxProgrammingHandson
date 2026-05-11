/*
 * memory_demo.c — Memory Allocation Internals
 *
 * Build: gcc -Wall -g memory_demo.c -o memory_demo
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

/* ============================================================
 * Demo 1: brk/sbrk — heap management
 * ============================================================ */
void demo_brk_sbrk(void) {
    printf("--- Demo 1: brk/sbrk Heap ---\n");

    void *initial = sbrk(0);
    printf("  Initial program break: %p\n", initial);

    /* Extend heap by 4096 bytes */
    void *block = sbrk(4096);
    printf("  After sbrk(4096):      %p\n", sbrk(0));
    printf("  Allocated block at:    %p\n", block);

    /* Write to the allocated memory */
    memset(block, 'A', 4096);
    printf("  Wrote 4096 bytes to heap.\n");

    /* Shrink heap back */
    sbrk(-4096);
    printf("  After sbrk(-4096):     %p\n", sbrk(0));
}

/* ============================================================
 * Demo 2: malloc behavior — small vs large
 * ============================================================ */
void demo_malloc_sizes(void) {
    printf("\n--- Demo 2: malloc Small vs Large ---\n");

    void *small = malloc(64);
    void *medium = malloc(1024);
    void *large = malloc(256 * 1024);     /* > 128KB → mmap */
    void *huge = malloc(10 * 1024 * 1024); /* 10MB → mmap */

    printf("  small  (64 B):   %p\n", small);
    printf("  medium (1 KB):   %p\n", medium);
    printf("  large  (256 KB): %p  (likely mmap'd)\n", large);
    printf("  huge   (10 MB):  %p  (likely mmap'd)\n", huge);

    /* Notice the address gap between heap and mmap regions */
    printf("\n  Heap region:   around %p\n", small);
    printf("  Mmap region:   around %p\n", large);

    free(small);
    free(medium);
    free(large);
    free(huge);
}

/* ============================================================
 * Demo 3: /proc/self/maps
 * ============================================================ */
void demo_proc_maps(void) {
    printf("\n--- Demo 3: Memory Maps (/proc/self/maps) ---\n");

    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) { perror("fopen"); return; }

    char line[512];
    int count = 0;
    printf("  %-36s %-5s %-10s %s\n", "Address Range", "Perms", "Offset", "Path");
    while (fgets(line, sizeof(line), fp) && count < 15) {
        printf("  %s", line);
        count++;
    }
    printf("  ... (truncated, run 'cat /proc/self/maps' for full output)\n");
    fclose(fp);
}

/* ============================================================
 * Demo 4: mmap anonymous memory
 * ============================================================ */
void demo_anon_mmap(void) {
    printf("\n--- Demo 4: Anonymous mmap ---\n");

    size_t size = 4 * 1024 * 1024;  /* 4MB */
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { perror("mmap"); return; }

    printf("  Mapped 4 MB at %p\n", mem);

    /* madvise hints */
    madvise(mem, size, MADV_SEQUENTIAL);
    printf("  MADV_SEQUENTIAL hint set.\n");

    /* Write pattern */
    memset(mem, 0x42, size);
    printf("  Wrote 4 MB pattern.\n");

    /* Tell kernel we don't need the data anymore */
    madvise(mem, size, MADV_DONTNEED);
    printf("  MADV_DONTNEED — pages reclaimed by kernel.\n");

    munmap(mem, size);
    printf("  Unmapped.\n");
}

/* ============================================================
 * Demo 5: mlock — pin pages in RAM
 * ============================================================ */
void demo_mlock(void) {
    printf("\n--- Demo 5: mlock (Pin Pages) ---\n");

    size_t size = 1024 * 1024;  /* 1MB */
    void *mem = malloc(size);

    if (mlock(mem, size) == 0) {
        printf("  Locked 1 MB in RAM (won't be swapped).\n");
        munlock(mem, size);
        printf("  Unlocked.\n");
    } else {
        perror("  mlock (may need 'ulimit -l unlimited' or root)");
    }

    free(mem);
}

/* ============================================================
 * Demo 6: VmRSS / VmSize from /proc/self/status
 * ============================================================ */
void demo_vmstats(void) {
    printf("\n--- Demo 6: Virtual Memory Stats ---\n");

    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) { perror("fopen"); return; }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Vm", 2) == 0 ||
            strncmp(line, "Rss", 3) == 0 ||
            strncmp(line, "Threads", 7) == 0) {
            printf("  %s", line);
        }
    }
    fclose(fp);
}

int main(void) {
    printf("=== Memory Management Demo ===\n\n");

    demo_brk_sbrk();
    demo_malloc_sizes();
    demo_proc_maps();
    demo_anon_mmap();
    demo_mlock();
    demo_vmstats();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
