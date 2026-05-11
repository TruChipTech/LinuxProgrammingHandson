/*
 * tempfile_demo.c — Secure vs Insecure Temporary File Creation
 *
 * Build: gcc -Wall -g tempfile_demo.c -o tempfile_demo
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

/* ============================================================
 * Demo 1: mkstemp — secure temp file
 * ============================================================ */
void demo_mkstemp(void) {
    printf("--- Demo 1: mkstemp (Secure) ---\n");

    char template[] = "/tmp/myapp_XXXXXX";
    int fd = mkstemp(template);
    if (fd < 0) { perror("mkstemp"); return; }

    printf("  Created: %s (fd=%d)\n", template);

    /* Check permissions */
    struct stat st;
    fstat(fd, &st);
    printf("  Permissions: %o (should be 0600)\n", st.st_mode & 0777);

    /* Write data */
    const char *data = "Sensitive temporary data\n";
    write(fd, data, strlen(data));
    printf("  Wrote %zu bytes.\n", strlen(data));

    /* Unlink immediately — file stays open but invisible */
    unlink(template);
    printf("  Unlinked (file open but name removed from filesystem).\n");

    /* Can still read/write via fd */
    lseek(fd, 0, SEEK_SET);
    char buf[64];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    printf("  Read back: '%s'\n", buf);

    close(fd);
    printf("  Closed — file now completely gone.\n");
}

/* ============================================================
 * Demo 2: mkostemp — with flags
 * ============================================================ */
void demo_mkostemp(void) {
    printf("\n--- Demo 2: mkostemp (with O_CLOEXEC) ---\n");

    char template[] = "/tmp/myapp_XXXXXX";
    int fd = mkostemp(template, O_CLOEXEC | O_APPEND);
    if (fd < 0) { perror("mkostemp"); return; }

    printf("  Created: %s (O_CLOEXEC | O_APPEND)\n", template);

    /* O_CLOEXEC means fd is automatically closed on exec() */
    int flags = fcntl(fd, F_GETFD);
    printf("  FD_CLOEXEC set: %s\n", (flags & FD_CLOEXEC) ? "YES" : "NO");

    unlink(template);
    close(fd);
}

/* ============================================================
 * Demo 3: mkdtemp — secure temp directory
 * ============================================================ */
void demo_mkdtemp(void) {
    printf("\n--- Demo 3: mkdtemp (Secure Directory) ---\n");

    char template[] = "/tmp/myapp_dir_XXXXXX";
    char *dir = mkdtemp(template);
    if (!dir) { perror("mkdtemp"); return; }

    printf("  Created directory: %s\n", dir);

    struct stat st;
    stat(dir, &st);
    printf("  Permissions: %o (should be 0700)\n", st.st_mode & 0777);

    /* Create a file inside */
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/data.txt", dir);
    FILE *fp = fopen(filepath, "w");
    if (fp) {
        fprintf(fp, "Temp data in secure directory\n");
        fclose(fp);
        printf("  Created file inside: %s\n", filepath);
    }

    /* Cleanup */
    unlink(filepath);
    rmdir(dir);
    printf("  Cleaned up directory.\n");
}

/* ============================================================
 * Demo 4: tmpfile — anonymous temp file
 * ============================================================ */
void demo_tmpfile(void) {
    printf("\n--- Demo 4: tmpfile (Anonymous) ---\n");

    FILE *fp = tmpfile();
    if (!fp) { perror("tmpfile"); return; }

    fprintf(fp, "This data has no filename on disk!\n");
    printf("  Wrote to anonymous temp file.\n");

    /* Read back */
    rewind(fp);
    char buf[64];
    if (fgets(buf, sizeof(buf), fp)) {
        printf("  Read back: '%s'\n", buf);
    }

    fclose(fp);
    printf("  Closed — automatically deleted.\n");
}

/* ============================================================
 * Demo 5: O_TMPFILE — Linux anonymous file
 * ============================================================ */
void demo_o_tmpfile(void) {
    printf("\n--- Demo 5: O_TMPFILE (Linux 3.11+) ---\n");

#ifdef O_TMPFILE
    int fd = open("/tmp", O_TMPFILE | O_RDWR, 0600);
    if (fd < 0) {
        if (errno == EOPNOTSUPP) {
            printf("  O_TMPFILE not supported on this filesystem.\n");
        } else {
            perror("  open O_TMPFILE");
        }
        return;
    }

    printf("  Created anonymous file (fd=%d, no name on disk!)\n", fd);

    const char *data = "Ultra-secure: this file NEVER had a name.\n";
    write(fd, data, strlen(data));

    lseek(fd, 0, SEEK_SET);
    char buf[128];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    printf("  Content: '%s'\n", buf);

    /* Optional: give it a name with linkat (makes it permanent) */
    /* linkat(fd, "", AT_FDCWD, "/tmp/promoted_file", AT_EMPTY_PATH); */

    close(fd);
    printf("  Closed — gone forever.\n");
#else
    printf("  O_TMPFILE not available on this system.\n");
#endif
}

/* ============================================================
 * Demo 6: The TOCTOU vulnerability
 * ============================================================ */
void demo_toctou_warning(void) {
    printf("\n--- Demo 6: TOCTOU Vulnerability (Educational) ---\n");

    printf("  INSECURE pattern (DO NOT USE):\n");
    printf("    char *name = tmpnam(NULL);    // Generate name\n");
    printf("    // <<< RACE WINDOW: attacker creates symlink here >>>\n");
    printf("    FILE *fp = fopen(name, \"w\"); // Open — follows symlink!\n");
    printf("\n");
    printf("  SECURE alternative:\n");
    printf("    int fd = mkstemp(template);   // Create + open atomically\n");
    printf("    unlink(template);             // Remove name immediately\n");
    printf("    // Work with fd — no race window!\n");
}

int main(void) {
    printf("=== Secure Temporary File Demo ===\n\n");

    demo_mkstemp();
    demo_mkostemp();
    demo_mkdtemp();
    demo_tmpfile();
    demo_o_tmpfile();
    demo_toctou_warning();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
