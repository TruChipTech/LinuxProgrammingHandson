/*
 * user_info.c — User and Group Information & Privilege Dropping
 *
 * Build: gcc -Wall -g user_info.c -o user_info
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

/* ============================================================
 * Demo 1: Current identity
 * ============================================================ */
void demo_current_identity(void) {
    printf("--- Demo 1: Current Identity ---\n");

    uid_t uid  = getuid();
    uid_t euid = geteuid();
    gid_t gid  = getgid();
    gid_t egid = getegid();

    printf("  Real UID:      %d\n", uid);
    printf("  Effective UID: %d\n", euid);
    printf("  Real GID:      %d\n", gid);
    printf("  Effective GID: %d\n", egid);
    printf("  Running as root: %s\n", euid == 0 ? "YES" : "NO");

    /* Lookup user info */
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        printf("\n  Username:  %s\n", pw->pw_name);
        printf("  Home:      %s\n", pw->pw_dir);
        printf("  Shell:     %s\n", pw->pw_shell);
        printf("  GECOS:     %s\n", pw->pw_gecos);
    }
}

/* ============================================================
 * Demo 2: Group information
 * ============================================================ */
void demo_groups(void) {
    printf("\n--- Demo 2: Group Information ---\n");

    gid_t gid = getgid();
    struct group *gr = getgrgid(gid);
    if (gr) {
        printf("  Primary group: %s (GID=%d)\n", gr->gr_name, gid);
    }

    /* Supplementary groups */
    int ngroups = getgroups(0, NULL);
    if (ngroups > 0) {
        gid_t *groups = malloc(ngroups * sizeof(gid_t));
        getgroups(ngroups, groups);

        printf("  Supplementary groups (%d):\n", ngroups);
        for (int i = 0; i < ngroups; i++) {
            struct group *g = getgrgid(groups[i]);
            printf("    GID=%-6d %s\n", groups[i],
                   g ? g->gr_name : "(unknown)");
        }
        free(groups);
    }
}

/* ============================================================
 * Demo 3: Enumerate users
 * ============================================================ */
void demo_enumerate_users(void) {
    printf("\n--- Demo 3: System Users (first 10) ---\n");
    printf("  %-12s %-6s %-6s %s\n", "Username", "UID", "GID", "Home");
    printf("  %-12s %-6s %-6s %s\n", "--------", "---", "---", "----");

    struct passwd *pw;
    int count = 0;
    setpwent();
    while ((pw = getpwent()) != NULL && count < 10) {
        printf("  %-12s %-6d %-6d %s\n",
               pw->pw_name, pw->pw_uid, pw->pw_gid, pw->pw_dir);
        count++;
    }
    endpwent();
}

/* ============================================================
 * Demo 4: Enumerate groups
 * ============================================================ */
void demo_enumerate_groups(void) {
    printf("\n--- Demo 4: System Groups (first 10) ---\n");
    printf("  %-16s %-6s %s\n", "Group", "GID", "Members");
    printf("  %-16s %-6s %s\n", "-----", "---", "-------");

    struct group *gr;
    int count = 0;
    setgrent();
    while ((gr = getgrent()) != NULL && count < 10) {
        printf("  %-16s %-6d ", gr->gr_name, gr->gr_gid);
        if (gr->gr_mem[0]) {
            for (int i = 0; gr->gr_mem[i]; i++) {
                printf("%s%s", i ? "," : "", gr->gr_mem[i]);
            }
        } else {
            printf("(none)");
        }
        printf("\n");
        count++;
    }
    endgrent();
}

/* ============================================================
 * Demo 5: Lookup by name
 * ============================================================ */
void demo_lookup(void) {
    printf("\n--- Demo 5: Lookups ---\n");

    /* Lookup root */
    struct passwd *pw = getpwnam("root");
    if (pw) {
        printf("  root: UID=%d, GID=%d, shell=%s\n",
               pw->pw_uid, pw->pw_gid, pw->pw_shell);
    }

    /* Lookup 'nobody' */
    pw = getpwnam("nobody");
    if (pw) {
        printf("  nobody: UID=%d, GID=%d, shell=%s\n",
               pw->pw_uid, pw->pw_gid, pw->pw_shell);
    }

    /* Lookup group 'root' */
    struct group *gr = getgrnam("root");
    if (gr) {
        printf("  Group 'root': GID=%d\n", gr->gr_gid);
    }
}

int main(void) {
    printf("=== User & Group Information Demo ===\n\n");

    demo_current_identity();
    demo_groups();
    demo_enumerate_users();
    demo_enumerate_groups();
    demo_lookup();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
