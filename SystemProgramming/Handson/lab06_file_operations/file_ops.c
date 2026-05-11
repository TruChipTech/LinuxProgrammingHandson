/*
 * file_ops.c — Low-Level POSIX File Operations
 *
 * Demonstrates open/read/write/lseek/close/fstat/ftruncate/fcntl.
 *
 * Build: gcc -Wall -g file_ops.c -o file_ops
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define TEST_FILE "/tmp/file_ops_test.dat"

/* ============================================================
 * Demo 1: Basic open/write/read/close
 * ============================================================ */
void demo_basic_io(void) {
    printf("--- Demo 1: Basic I/O ---\n");

    /* Create and write */
    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return; }

    const char *msg = "Hello from low-level I/O!\n";
    ssize_t written = write(fd, msg, strlen(msg));
    printf("  Written: %zd bytes\n", written);
    close(fd);

    /* Read back */
    fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    char buf[256];
    ssize_t nread = read(fd, buf, sizeof(buf) - 1);
    buf[nread] = '\0';
    printf("  Read:    %zd bytes => '%s'\n", nread, buf);
    close(fd);
}

/* ============================================================
 * Demo 2: lseek — random access
 * ============================================================ */
void demo_seek(void) {
    printf("\n--- Demo 2: lseek Random Access ---\n");

    int fd = open(TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return; }

    /* Write records at fixed positions */
    for (int i = 0; i < 5; i++) {
        char record[32];
        snprintf(record, sizeof(record), "Record-%d", i);
        lseek(fd, i * 32, SEEK_SET);
        write(fd, record, 32);
    }

    /* Read record #3 directly */
    lseek(fd, 3 * 32, SEEK_SET);
    char buf[32];
    read(fd, buf, 32);
    printf("  Record at offset 96: '%s'\n", buf);

    /* Get current position and file size */
    off_t pos = lseek(fd, 0, SEEK_CUR);
    off_t size = lseek(fd, 0, SEEK_END);
    printf("  Current position: %ld\n", (long)pos);
    printf("  File size:        %ld bytes\n", (long)size);

    close(fd);
}

/* ============================================================
 * Demo 3: fstat — file metadata
 * ============================================================ */
void demo_fstat(void) {
    printf("\n--- Demo 3: fstat Metadata ---\n");

    int fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat"); close(fd); return; }

    printf("  Device:     %ld\n", (long)st.st_dev);
    printf("  Inode:      %ld\n", (long)st.st_ino);
    printf("  Mode:       %o\n", st.st_mode & 0777);
    printf("  Links:      %ld\n", (long)st.st_nlink);
    printf("  UID/GID:    %d/%d\n", st.st_uid, st.st_gid);
    printf("  Size:       %ld bytes\n", (long)st.st_size);
    printf("  Block size: %ld\n", (long)st.st_blksize);
    printf("  Blocks:     %ld\n", (long)st.st_blocks);
    printf("  Modified:   %s", ctime(&st.st_mtime));

    close(fd);
}

/* ============================================================
 * Demo 4: O_APPEND — atomic append
 * ============================================================ */
void demo_append(void) {
    printf("\n--- Demo 4: O_APPEND ---\n");

    /* Write initial content */
    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, "Line 1\n", 7);
    close(fd);

    /* Append with O_APPEND flag */
    fd = open(TEST_FILE, O_WRONLY | O_APPEND);
    write(fd, "Line 2 (appended)\n", 18);
    write(fd, "Line 3 (appended)\n", 18);
    close(fd);

    /* Read and display */
    fd = open(TEST_FILE, O_RDONLY);
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    printf("  File contents:\n%s", buf);
    close(fd);
}

/* ============================================================
 * Demo 5: fcntl file locking
 * ============================================================ */
void demo_locking(void) {
    printf("\n--- Demo 5: fcntl File Locking ---\n");

    int fd = open(TEST_FILE, O_RDWR);
    if (fd < 0) { perror("open"); return; }

    /* Try to get an exclusive lock on the first 100 bytes */
    struct flock fl = {
        .l_type   = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start  = 0,
        .l_len    = 100,
    };

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        printf("  Cannot lock file: %s\n", strerror(errno));
    } else {
        printf("  Write lock acquired on bytes 0-99\n");
    }

    /* Check who holds the lock */
    fl.l_type = F_WRLCK;
    fcntl(fd, F_GETLK, &fl);
    if (fl.l_type == F_UNLCK) {
        printf("  No conflicting lock found\n");
    } else {
        printf("  Lock held by PID %d\n", fl.l_pid);
    }

    /* Release lock */
    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);
    printf("  Lock released\n");

    close(fd);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== POSIX File Operations Demo ===\n\n");

    demo_basic_io();
    demo_seek();
    demo_fstat();
    demo_append();
    demo_locking();

    /* Cleanup */
    unlink(TEST_FILE);

    printf("\n=== Demo Complete ===\n");
    return 0;
}
