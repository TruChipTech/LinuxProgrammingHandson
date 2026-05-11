/*
 * process_lab.c — Process Management Lab
 *
 * Demonstrates fork(), exec(), wait(), getpid/getppid, and process trees.
 *
 * Build: gcc -Wall -g process_lab.c -o process_lab
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* ============================================================
 * Demo 1: Basic fork
 * ============================================================ */
void demo_basic_fork(void) {
    printf("--- Demo 1: Basic fork() ---\n");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        /* Child */
        printf("  [Child]  PID=%d  PPID=%d\n", getpid(), getppid());
        _exit(42);
    } else {
        /* Parent */
        printf("  [Parent] PID=%d  Child=%d\n", getpid(), pid);
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("  [Parent] Child exited with code %d\n",
                   WEXITSTATUS(status));
        }
    }
}

/* ============================================================
 * Demo 2: fork + exec
 * ============================================================ */
void demo_fork_exec(void) {
    printf("\n--- Demo 2: fork() + exec() ---\n");

    pid_t pid = fork();

    if (pid == 0) {
        /* Child: replace with 'ls -la /tmp' */
        printf("  [Child]  About to exec 'ls -la /tmp'...\n");
        execlp("ls", "ls", "-la", "/tmp", NULL);
        /* If exec fails: */
        perror("execlp");
        _exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        printf("  [Parent] exec'd process exited with code %d\n",
               WEXITSTATUS(status));
    }
}

/* ============================================================
 * Demo 3: Multiple children
 * ============================================================ */
void demo_multiple_children(void) {
    printf("\n--- Demo 3: Multiple Children ---\n");

    int num_children = 4;

    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            /* Child work */
            printf("  [Child %d] PID=%d starting work...\n", i, getpid());
            usleep(100000 * (i + 1));  /* Stagger completion */
            printf("  [Child %d] PID=%d done.\n", i, getpid());
            _exit(i);
        }
    }

    /* Parent: wait for all children */
    printf("  [Parent] Waiting for %d children...\n", num_children);
    int status;
    pid_t wpid;
    while ((wpid = wait(&status)) > 0) {
        printf("  [Parent] Child PID=%d exited with code %d\n",
               wpid, WEXITSTATUS(status));
    }
}

/* ============================================================
 * Demo 4: Process chain (grandchild)
 * ============================================================ */
void demo_process_chain(void) {
    printf("\n--- Demo 4: Process Chain ---\n");

    printf("  [Generation 0] PID=%d\n", getpid());

    pid_t p1 = fork();
    if (p1 == 0) {
        printf("  [Generation 1] PID=%d  PPID=%d\n", getpid(), getppid());

        pid_t p2 = fork();
        if (p2 == 0) {
            printf("  [Generation 2] PID=%d  PPID=%d\n",
                   getpid(), getppid());
            _exit(0);
        }
        waitpid(p2, NULL, 0);
        _exit(0);
    }
    waitpid(p1, NULL, 0);
}

/* ============================================================
 * Demo 5: Environment variables
 * ============================================================ */
void demo_environment(void) {
    printf("\n--- Demo 5: Environment Variables ---\n");

    /* Set an env var before fork */
    setenv("MY_CUSTOM_VAR", "HelloFromParent", 1);

    pid_t pid = fork();
    if (pid == 0) {
        /* Child inherits environment */
        const char *val = getenv("MY_CUSTOM_VAR");
        printf("  [Child]  MY_CUSTOM_VAR = '%s'\n", val ? val : "(null)");

        /* Modify in child — doesn't affect parent */
        setenv("MY_CUSTOM_VAR", "ModifiedByChild", 1);
        printf("  [Child]  Modified to '%s'\n", getenv("MY_CUSTOM_VAR"));
        _exit(0);
    }

    waitpid(pid, NULL, 0);
    printf("  [Parent] MY_CUSTOM_VAR = '%s' (unchanged)\n",
           getenv("MY_CUSTOM_VAR"));
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Process Management Lab ===\n\n");

    demo_basic_fork();
    demo_fork_exec();
    demo_multiple_children();
    demo_process_chain();
    demo_environment();

    printf("\n=== Lab Complete ===\n");
    return 0;
}
