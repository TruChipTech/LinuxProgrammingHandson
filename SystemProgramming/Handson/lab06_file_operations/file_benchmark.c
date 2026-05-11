/*
 * file_benchmark.c — I/O Performance Benchmark
 *
 * Compares read/write performance across different buffer sizes
 * and compares buffered (stdio) vs unbuffered (POSIX) I/O.
 *
 * Build: gcc -Wall -O2 file_benchmark.c -o file_benchmark
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define TEST_FILE "/tmp/benchmark_test.dat"
#define DATA_SIZE (64 * 1024 * 1024)  /* 64 MB */

static double elapsed_ms(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000.0 +
           (end->tv_nsec - start->tv_nsec) / 1000000.0;
}

/* ============================================================
 * Benchmark: POSIX write with various buffer sizes
 * ============================================================ */
void benchmark_posix_write(size_t buf_size) {
    char *buf = malloc(buf_size);
    memset(buf, 'X', buf_size);

    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); free(buf); return; }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t remaining = DATA_SIZE;
    while (remaining > 0) {
        size_t chunk = (remaining < buf_size) ? remaining : buf_size;
        write(fd, buf, chunk);
        remaining -= chunk;
    }

    fsync(fd);
    clock_gettime(CLOCK_MONOTONIC, &end);
    close(fd);

    double ms = elapsed_ms(&start, &end);
    double mbps = (DATA_SIZE / (1024.0 * 1024.0)) / (ms / 1000.0);
    printf("  POSIX write  buf=%6zu  =>  %8.1f ms  (%6.1f MB/s)\n",
           buf_size, ms, mbps);

    free(buf);
}

/* ============================================================
 * Benchmark: POSIX read with various buffer sizes
 * ============================================================ */
void benchmark_posix_read(size_t buf_size) {
    char *buf = malloc(buf_size);

    int fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0) { perror("open"); free(buf); return; }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (read(fd, buf, buf_size) > 0)
        ;

    clock_gettime(CLOCK_MONOTONIC, &end);
    close(fd);

    double ms = elapsed_ms(&start, &end);
    double mbps = (DATA_SIZE / (1024.0 * 1024.0)) / (ms / 1000.0);
    printf("  POSIX read   buf=%6zu  =>  %8.1f ms  (%6.1f MB/s)\n",
           buf_size, ms, mbps);

    free(buf);
}

/* ============================================================
 * Benchmark: stdio (buffered) write
 * ============================================================ */
void benchmark_stdio_write(void) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    FILE *fp = fopen(TEST_FILE, "w");
    if (!fp) { perror("fopen"); return; }

    for (size_t i = 0; i < DATA_SIZE; i += 80) {
        /* Write 80 chars at a time via fwrite */
        char line[80];
        memset(line, 'S', 80);
        fwrite(line, 1, 80, fp);
    }

    fflush(fp);
    fclose(fp);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double ms = elapsed_ms(&start, &end);
    double mbps = (DATA_SIZE / (1024.0 * 1024.0)) / (ms / 1000.0);
    printf("  stdio write (80-byte fwrite) =>  %8.1f ms  (%6.1f MB/s)\n",
           ms, mbps);
}

/* ============================================================
 * Benchmark: byte-at-a-time (worst case)
 * ============================================================ */
void benchmark_byte_write(void) {
    size_t small_size = 1024 * 1024;  /* Only 1 MB — too slow otherwise */

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    for (size_t i = 0; i < small_size; i++) {
        char c = 'B';
        write(fd, &c, 1);
    }
    fsync(fd);
    close(fd);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double ms = elapsed_ms(&start, &end);
    double mbps = (small_size / (1024.0 * 1024.0)) / (ms / 1000.0);
    printf("  Byte-at-a-time write (1 MB)  =>  %8.1f ms  (%6.1f MB/s)\n",
           ms, mbps);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== I/O Performance Benchmark ===\n");
    printf("  Data size: %d MB\n\n", DATA_SIZE / (1024 * 1024));

    /* Write benchmarks */
    printf("[WRITE Tests]\n");
    size_t sizes[] = { 64, 512, 4096, 16384, 65536, 262144 };
    int n = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < n; i++) {
        benchmark_posix_write(sizes[i]);
    }
    benchmark_stdio_write();
    benchmark_byte_write();

    /* Prepare file for read tests */
    printf("\n[READ Tests]\n");
    benchmark_posix_write(65536);  /* Write test file */
    for (int i = 0; i < n; i++) {
        benchmark_posix_read(sizes[i]);
    }

    /* Cleanup */
    unlink(TEST_FILE);

    printf("\n=== Benchmark Complete ===\n");
    return 0;
}
