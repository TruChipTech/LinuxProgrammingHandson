/*
 * zombie_creator.c — Demonstrate zombie and orphan processes
 *
 * Build: gcc -Wall -g zombie_creator.c -o zombie_creator
 *
 * Run and observe with: ps aux | grep zombie
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* ============================================================
 * Demo 1: Create a zombie process
 * ============================================================ */
void demo_zombie(void) {
    printf("--- Demo 1: Zombie Process ---\n");

    pid_t pid = fork();

    if (pid == 0) {
        /* Child exits immediately */
        printf("  [Child]  PID=%d exiting now...\n", getpid());
        _exit(0);
    }

    /* Parent does NOT call wait() — child becomes zombie */
    printf("  [Parent] PID=%d, Child=%d\n", getpid(), pid);
    printf("  [Parent] NOT calling wait() — child is now a zombie.\n");
    printf("  [Parent] Run: ps aux | grep %d\n", pid);
    printf("  [Parent] You should see '<defunct>' in the output.\n");
    printf("  [Parent] Sleeping 10 seconds to observe...\n");
    sleep(10);

    /* Now reap the zombie */
    int status;
    waitpid(pid, &status, 0);
    printf("  [Parent] Reaped zombie. Exit code: %d\n",
           WEXITSTATUS(status));
}

/* ============================================================
 * Demo 2: Create an orphan process
 * ============================================================ */
void demo_orphan(void) {
    printf("\n--- Demo 2: Orphan Process ---\n");

    pid_t pid = fork();

    if (pid == 0) {
        /* Child sleeps, parent exits first */
        printf("  [Child]  PID=%d, PPID=%d (before orphan)\n",
               getpid(), getppid());
        sleep(3);
        printf("  [Child]  PID=%d, PPID=%d (after orphan — adopted by init)\n",
               getpid(), getppid());
        _exit(0);
    }

    /* Parent exits immediately */
    printf("  [Parent] PID=%d exiting, leaving child %d as orphan.\n",
           getpid(), pid);
    /* Don't call _exit() here since we're in a demo function */
    sleep(1);  /* Small delay so child can print */
}

/* ============================================================
 * Demo 3: Proper zombie prevention with SIGCHLD
 * ============================================================ */
#include <signal.h>

void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Silently reap children */
    }
}

void demo_no_zombie(void) {
    printf("\n--- Demo 3: Zombie Prevention with SIGCHLD ---\n");

    /* Install SIGCHLD handler */
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    /* Create several children */
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            printf("  [Child %d] PID=%d exiting...\n", i, getpid());
            _exit(i);
        }
        printf("  [Parent] Spawned child %d (PID=%d)\n", i, pid);
    }

    /* Give children time to exit and be reaped */
    sleep(2);
    printf("  [Parent] No zombies — all children reaped by SIGCHLD handler.\n");
    printf("  [Parent] Verify: ps aux | grep defunct | grep -v grep\n");

    /* Restore default */
    signal(SIGCHLD, SIG_DFL);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(int argc, char *argv[]) {
    printf("=== Zombie & Orphan Process Demo ===\n\n");

    if (argc > 1 && argv[1][0] == '1') {
        demo_zombie();
    } else if (argc > 1 && argv[1][0] == '2') {
        demo_orphan();
    } else if (argc > 1 && argv[1][0] == '3') {
        demo_no_zombie();
    } else {
        printf("Usage: %s <demo_number>\n", argv[0]);
        printf("  1 — Zombie process demo (10 second wait)\n");
        printf("  2 — Orphan process demo\n");
        printf("  3 — Zombie prevention with SIGCHLD\n");
    }

    printf("\n=== Demo Complete ===\n");
    return 0;
}
