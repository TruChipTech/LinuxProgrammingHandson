/*
 * priv_drop.c — Privilege Dropping Demo
 *
 * Build: gcc -Wall -g priv_drop.c -o priv_drop
 * Run:   sudo ./priv_drop
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

static void print_ids(const char *label) {
    printf("  [%s] UID=%d EUID=%d GID=%d EGID=%d\n",
           label, getuid(), geteuid(), getgid(), getegid());
}

/* Safely drop privileges to a target user */
int drop_privileges(const char *username) {
    struct passwd *pw = getpwnam(username);
    if (!pw) {
        fprintf(stderr, "  User '%s' not found\n", username);
        return -1;
    }

    printf("  Dropping to user '%s' (UID=%d, GID=%d)\n",
           username, pw->pw_uid, pw->pw_gid);

    /* Step 1: Set supplementary groups */
    if (initgroups(username, pw->pw_gid) < 0) {
        perror("  initgroups");
        return -1;
    }
    printf("  [OK] Supplementary groups set\n");

    /* Step 2: Set GID (must do before UID!) */
    if (setgid(pw->pw_gid) < 0) {
        perror("  setgid");
        return -1;
    }
    printf("  [OK] GID set to %d\n", pw->pw_gid);

    /* Step 3: Set UID (this is permanent — can't go back) */
    if (setuid(pw->pw_uid) < 0) {
        perror("  setuid");
        return -1;
    }
    printf("  [OK] UID set to %d\n", pw->pw_uid);

    /* Step 4: Verify we can't escalate back */
    if (setuid(0) == 0) {
        fprintf(stderr, "  [FAIL] Was able to escalate back to root!\n");
        return -1;
    }
    printf("  [OK] Cannot escalate back to root (errno=%d: %s)\n",
           errno, strerror(errno));

    return 0;
}

int main(void) {
    printf("=== Privilege Dropping Demo ===\n\n");

    print_ids("START");

    if (geteuid() != 0) {
        printf("\n  Not running as root. Run with: sudo %s\n", "priv_drop");
        printf("  Showing what WOULD happen...\n\n");

        printf("  Privilege drop order (important!):\n");
        printf("    1. initgroups() — set supplementary groups\n");
        printf("    2. setgid()     — change group ID\n");
        printf("    3. setuid()     — change user ID (PERMANENT)\n");
        printf("\n  WHY this order?\n");
        printf("    - setuid() to non-root is irreversible\n");
        printf("    - Can't call setgid() after dropping UID (no permission)\n");
        printf("    - Can't call initgroups() after dropping UID\n");

        printf("\n=== Demo Complete ===\n");
        return 0;
    }

    /* Demonstrate privileged operation */
    printf("\n--- Privileged Operation ---\n");
    FILE *fp = fopen("/etc/shadow", "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            printf("  /etc/shadow first line: %.40s...\n", line);
        }
        fclose(fp);
    }

    /* Drop privileges */
    printf("\n--- Dropping Privileges ---\n");
    if (drop_privileges("nobody") < 0) {
        fprintf(stderr, "  Failed to drop privileges!\n");
        return 1;
    }

    print_ids("AFTER DROP");

    /* Try privileged operation again */
    printf("\n--- Attempting Privileged Operation After Drop ---\n");
    fp = fopen("/etc/shadow", "r");
    if (fp) {
        printf("  [FAIL] Could still read /etc/shadow!\n");
        fclose(fp);
    } else {
        printf("  [OK] Cannot read /etc/shadow: %s\n", strerror(errno));
    }

    printf("\n=== Demo Complete ===\n");
    return 0;
}
