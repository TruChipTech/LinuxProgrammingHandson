/*
 * signal_handler.c — Signal Handling Demo
 *
 * Demonstrates sigaction, signal masks, custom handlers, and SA_RESTART.
 *
 * Build: gcc -Wall -g signal_handler.c -o signal_handler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

static volatile sig_atomic_t got_sigint  = 0;
static volatile sig_atomic_t got_sigusr1 = 0;
static volatile sig_atomic_t got_sigusr2 = 0;

/* ============================================================
 * Signal handlers
 * ============================================================ */
void handle_sigint(int sig) {
    (void)sig;
    got_sigint++;
    const char msg[] = "\n[HANDLER] SIGINT received! (Ctrl+C)\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

void handle_sigusr1(int sig, siginfo_t *info, void *ucontext) {
    (void)sig;
    (void)ucontext;
    got_sigusr1++;
    char msg[128];
    int len = snprintf(msg, sizeof(msg),
                       "[HANDLER] SIGUSR1 from PID=%d\n", info->si_pid);
    write(STDOUT_FILENO, msg, len);
}

void handle_sigusr2(int sig) {
    (void)sig;
    got_sigusr2++;
    const char msg[] = "[HANDLER] SIGUSR2 received!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

/* ============================================================
 * Setup signal handlers using sigaction
 * ============================================================ */
void setup_handlers(void) {
    struct sigaction sa;

    /* SIGINT — simple handler */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  /* Restart interrupted syscalls */
    sigaction(SIGINT, &sa, NULL);

    /* SIGUSR1 — handler with siginfo_t */
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    /* SIGUSR2 — simple handler */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigusr2;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &sa, NULL);
}

/* ============================================================
 * Demo: Signal masking
 * ============================================================ */
void demo_signal_mask(void) {
    printf("\n--- Signal Masking Demo ---\n");

    sigset_t mask, old_mask;

    /* Block SIGUSR1 */
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);
    printf("  SIGUSR1 is now BLOCKED.\n");

    /* Send SIGUSR1 to self — it's pending but won't be delivered */
    printf("  Sending SIGUSR1 to self (PID=%d)...\n", getpid());
    kill(getpid(), SIGUSR1);
    printf("  Signal sent but handler NOT called (blocked).\n");

    /* Check pending signals */
    sigset_t pending;
    sigpending(&pending);
    if (sigismember(&pending, SIGUSR1)) {
        printf("  SIGUSR1 is PENDING.\n");
    }

    /* Unblock — handler fires immediately */
    printf("  Unblocking SIGUSR1...\n");
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    printf("  Handler should have fired above.\n");
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    printf("=== Signal Handling Demo ===\n");
    printf("PID: %d\n\n", getpid());

    setup_handlers();

    printf("Handlers installed for SIGINT, SIGUSR1, SIGUSR2.\n");
    printf("Try these commands from another terminal:\n");
    printf("  kill -SIGUSR1 %d\n", getpid());
    printf("  kill -SIGUSR2 %d\n", getpid());
    printf("  Press Ctrl+C to send SIGINT\n\n");

    demo_signal_mask();

    printf("\n--- Waiting for signals (Ctrl+C three times to exit) ---\n");
    while (got_sigint < 3) {
        printf("  [main] Waiting... (SIGINT=%d, USR1=%d, USR2=%d)\n",
               (int)got_sigint, (int)got_sigusr1, (int)got_sigusr2);
        sleep(3);
    }

    printf("\n[main] Received 3 SIGINTs. Exiting.\n");
    printf("Signal counts: SIGINT=%d, SIGUSR1=%d, SIGUSR2=%d\n",
           (int)got_sigint, (int)got_sigusr1, (int)got_sigusr2);

    return 0;
}
