/*
 * mmap_demo.c — Memory-Mapped File I/O Demo
 *
 * Demonstrates mmap for reading/writing files and shared memory.
 *
 * Build: gcc -Wall -g mmap_demo.c -o mmap_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#define TEST_FILE "/tmp/mmap_test.dat"
#define MAP_SIZE  (4 * 1024 * 1024)  /* 4 MB */

/* ============================================================
 * Demo 1: Write via mmap
 * ============================================================ */
void demo_mmap_write(void) {
    printf("--- Demo 1: mmap Write ---\n");

    int fd = open(TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return; }

    /* Extend file to MAP_SIZE */
    if (ftruncate(fd, MAP_SIZE) < 0) {
        perror("ftruncate");
        close(fd);
        return;
    }

    /* Map the file */
    char *map = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }

    /* Write data directly to mapped memory */
    const char *header = "=== MMAP DATA ===\n";
    memcpy(map, header, strlen(header));

    for (int i = 0; i < 100; i++) {
        char line[64];
        int len = snprintf(line, sizeof(line), "Record %03d: value=%d\n",
                          i, i * 42);
        memcpy(map + strlen(header) + i * 64, line, len);
    }

    /* Sync to disk */
    msync(map, MAP_SIZE, MS_SYNC);
    printf("  Wrote %d records via mmap\n", 100);

    munmap(map, MAP_SIZE);
    close(fd);
}

/* ============================================================
 * Demo 2: Read via mmap
 * ============================================================ */
void demo_mmap_read(void) {
    printf("\n--- Demo 2: mmap Read ---\n");

    int fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    struct stat st;
    fstat(fd, &st);

    char *map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }

    /* Read first 200 bytes */
    char preview[201];
    size_t to_read = (st.st_size < 200) ? (size_t)st.st_size : 200;
    memcpy(preview, map, to_read);
    preview[to_read] = '\0';
    printf("  First 200 bytes:\n%s\n", preview);

    /* Search for a pattern in memory */
    const char *needle = "Record 042";
    char *found = memmem(map, st.st_size, needle, strlen(needle));
    if (found) {
        printf("  Found '%s' at offset %ld\n", needle, found - map);
    }

    munmap(map, st.st_size);
    close(fd);
}

/* ============================================================
 * Demo 3: mmap vs read() performance
 * ============================================================ */
void demo_mmap_vs_read(void) {
    printf("\n--- Demo 3: mmap vs read() Benchmark ---\n");

    /* Create a test file */
    int fd = open(TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
    ftruncate(fd, MAP_SIZE);
    char *tmp = mmap(NULL, MAP_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    memset(tmp, 'A', MAP_SIZE);
    munmap(tmp, MAP_SIZE);
    close(fd);

    struct timespec start, end;

    /* Test 1: read() */
    fd = open(TEST_FILE, O_RDONLY);
    char *buf = malloc(MAP_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &start);
    read(fd, buf, MAP_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    close(fd);
    free(buf);

    double read_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("  read():  %.2f ms\n", read_ms);

    /* Test 2: mmap + access */
    fd = open(TEST_FILE, O_RDONLY);
    clock_gettime(CLOCK_MONOTONIC, &start);
    char *mmap_buf = mmap(NULL, MAP_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    /* Touch every page to force page faults */
    volatile long sum = 0;
    for (int i = 0; i < MAP_SIZE; i += 4096) {
        sum += mmap_buf[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    munmap(mmap_buf, MAP_SIZE);
    close(fd);

    double mmap_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("  mmap():  %.2f ms\n", mmap_ms);

    printf("  Data: %d MB, sum=%ld (prevents optimization)\n",
           MAP_SIZE / (1024 * 1024), (long)sum);
}

/* ============================================================
 * Demo 4: Anonymous mmap (no file — pure shared memory)
 * ============================================================ */
void demo_anon_mmap(void) {
    printf("\n--- Demo 4: Anonymous mmap ---\n");

    /* Allocate 1 page of anonymous memory */
    size_t page = 4096;
    char *mem = mmap(NULL, page, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap anonymous");
        return;
    }

    sprintf(mem, "This is anonymous mmap memory!");
    printf("  Content: '%s'\n", mem);
    printf("  Address: %p\n", (void *)mem);

    munmap(mem, page);
    printf("  Memory unmapped.\n");
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Memory-Mapped I/O Demo ===\n\n");

    demo_mmap_write();
    demo_mmap_read();
    demo_mmap_vs_read();
    demo_anon_mmap();

    unlink(TEST_FILE);

    printf("\n=== Demo Complete ===\n");
    return 0;
}
