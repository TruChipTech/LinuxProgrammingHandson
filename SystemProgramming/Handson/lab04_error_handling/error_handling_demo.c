/*
 * error_handling_demo.c — Comprehensive Error Handling in Linux
 *
 * Demonstrates errno, perror, strerror, and proper error checking patterns.
 *
 * Build: gcc -Wall -g error_handling_demo.c -o error_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* ============================================================
 * Error handling wrapper macros
 * ============================================================ */

#define CHECK_FD(fd, msg) do { \
    if ((fd) < 0) { \
        fprintf(stderr, "[ERROR] %s:%d: %s: %s (errno=%d)\n", \
                __FILE__, __LINE__, (msg), strerror(errno), errno); \
        return -1; \
    } \
} while (0)

#define CHECK_PTR(ptr, msg) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "[ERROR] %s:%d: %s: %s (errno=%d)\n", \
                __FILE__, __LINE__, (msg), strerror(errno), errno); \
        return -1; \
    } \
} while (0)

/* ============================================================
 * Demo 1: File operation errors
 * ============================================================ */
int demo_file_errors(void) {
    printf("\n--- Demo 1: File Operation Errors ---\n");

    /* Error: File does not exist (ENOENT) */
    int fd = open("/tmp/nonexistent_file_xyz_123.txt", O_RDONLY);
    if (fd < 0) {
        printf("  open() failed:\n");
        printf("    errno value: %d\n", errno);
        printf("    errno name:  ENOENT (%d)\n", ENOENT);
        printf("    strerror():  %s\n", strerror(errno));
        printf("    perror():    ");
        perror("open");
    }

    /* Error: Permission denied (EACCES) */
    fd = open("/etc/shadow", O_RDONLY);
    if (fd < 0) {
        printf("\n  open(/etc/shadow) failed:\n");
        printf("    errno: %d (%s)\n", errno, strerror(errno));
    }

    /* Error: File exists (EEXIST) */
    fd = open("/tmp/error_demo_test.txt", O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd >= 0) {
        close(fd);
        /* Try again — should fail with EEXIST */
        fd = open("/tmp/error_demo_test.txt", O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd < 0 && errno == EEXIST) {
            printf("\n  Second open with O_EXCL failed as expected:\n");
            printf("    errno: %d (%s)\n", errno, strerror(errno));
        }
        unlink("/tmp/error_demo_test.txt");
    }

    return 0;
}

/* ============================================================
 * Demo 2: Memory allocation errors
 * ============================================================ */
int demo_memory_errors(void) {
    printf("\n--- Demo 2: Memory Allocation Errors ---\n");

    /* Normal allocation */
    void *ptr = malloc(1024);
    CHECK_PTR(ptr, "malloc(1024)");
    printf("  malloc(1024):      Success (ptr=%p)\n", ptr);
    free(ptr);

    /* Impossibly large allocation */
    errno = 0;
    ptr = malloc((size_t)-1);  /* Maximum size_t value */
    if (ptr == NULL) {
        printf("  malloc(SIZE_MAX):  Failed — %s (errno=%d)\n",
               strerror(errno), errno);
    } else {
        free(ptr);
    }

    /* calloc overflow check */
    errno = 0;
    ptr = calloc((size_t)1 << 62, (size_t)1 << 62);
    if (ptr == NULL) {
        printf("  calloc(huge):      Failed — %s\n", strerror(errno));
    } else {
        free(ptr);
    }

    return 0;
}

/* ============================================================
 * Demo 3: Proper error handling pattern
 * ============================================================ */
int safe_read_file(const char *path, char *buf, size_t bufsize) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -errno;  /* Return negative errno */
    }

    ssize_t nread = read(fd, buf, bufsize - 1);
    if (nread < 0) {
        int saved_errno = errno;  /* Save before close might change it */
        close(fd);
        return -saved_errno;
    }

    buf[nread] = '\0';
    close(fd);
    return (int)nread;
}

int demo_proper_pattern(void) {
    printf("\n--- Demo 3: Proper Error Handling Pattern ---\n");

    char buf[256];

    /* Success case */
    int ret = safe_read_file("/etc/hostname", buf, sizeof(buf));
    if (ret >= 0) {
        printf("  Read /etc/hostname: '%.*s' (%d bytes)\n",
               (int)(ret > 30 ? 30 : ret), buf, ret);
    } else {
        printf("  Read /etc/hostname failed: %s\n", strerror(-ret));
    }

    /* Failure case */
    ret = safe_read_file("/nonexistent", buf, sizeof(buf));
    if (ret >= 0) {
        printf("  Read /nonexistent: success (unexpected!)\n");
    } else {
        printf("  Read /nonexistent failed: %s (code %d)\n",
               strerror(-ret), ret);
    }

    return 0;
}

/* ============================================================
 * Demo 4: errno gotchas
 * ============================================================ */
int demo_errno_gotchas(void) {
    printf("\n--- Demo 4: errno Gotchas ---\n");

    /* GOTCHA 1: errno is only valid immediately after a failed call */
    open("/nonexistent", O_RDONLY);  /* Sets errno to ENOENT */
    printf("  After failed open: errno=%d (%s)\n", errno, strerror(errno));

    /* This successful call does NOT reset errno! */
    FILE *f = fopen("/etc/hostname", "r");
    printf("  After successful fopen: errno=%d (still %s!)\n",
           errno, strerror(errno));
    if (f) fclose(f);

    /* GOTCHA 2: Always check return value FIRST, then errno */
    printf("\n  Rule: Check return value first, errno second.\n");
    printf("  Rule: Always set errno=0 before calls that don't\n");
    printf("        have a clear error return value.\n");

    /* GOTCHA 3: errno is thread-local (safe in multithreaded code) */
    printf("\n  Note: errno is thread-local since POSIX.1c\n");

    return 0;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Linux Error Handling Demo ===\n");

    demo_file_errors();
    demo_memory_errors();
    demo_proper_pattern();
    demo_errno_gotchas();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
