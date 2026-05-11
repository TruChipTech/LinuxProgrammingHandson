/*
 * timer_demo.c — Linux Timer APIs
 *
 * Build: gcc -Wall -g timer_demo.c -o timer_demo -lrt -lpthread
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <errno.h>

/* ============================================================
 * Demo 1: Clock types
 * ============================================================ */
void demo_clocks(void) {
    printf("--- Demo 1: Available Clocks ---\n");

    struct {
        clockid_t id;
        const char *name;
    } clocks[] = {
        {CLOCK_REALTIME,           "CLOCK_REALTIME"},
        {CLOCK_MONOTONIC,          "CLOCK_MONOTONIC"},
        {CLOCK_MONOTONIC_RAW,      "CLOCK_MONOTONIC_RAW"},
        {CLOCK_PROCESS_CPUTIME_ID, "CLOCK_PROCESS_CPUTIME_ID"},
        {CLOCK_THREAD_CPUTIME_ID,  "CLOCK_THREAD_CPUTIME_ID"},
        {CLOCK_BOOTTIME,           "CLOCK_BOOTTIME"},
    };
    int n = sizeof(clocks) / sizeof(clocks[0]);

    for (int i = 0; i < n; i++) {
        struct timespec ts, res;
        if (clock_gettime(clocks[i].id, &ts) == 0) {
            clock_getres(clocks[i].id, &res);
            printf("  %-30s %ld.%09ld  (res: %ld ns)\n",
                   clocks[i].name, ts.tv_sec, ts.tv_nsec, res.tv_nsec);
        }
    }
}

/* ============================================================
 * Demo 2: nanosleep with interrupt handling
 * ============================================================ */
void demo_nanosleep(void) {
    printf("\n--- Demo 2: nanosleep ---\n");

    struct timespec req = {.tv_sec = 0, .tv_nsec = 500000000};  /* 500ms */
    struct timespec rem;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("  Sleeping 500ms...\n");
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        printf("  Interrupted! Remaining: %ld.%09ld\n", rem.tv_sec, rem.tv_nsec);
        req = rem;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double actual = (end.tv_sec - start.tv_sec) * 1000.0 +
                    (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("  Actual sleep: %.2f ms\n", actual);
}

/* ============================================================
 * Demo 3: POSIX timer (signal-based)
 * ============================================================ */
static volatile int timer_count = 0;

void timer_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig; (void)uc;
    timer_count++;
    int overrun = timer_getoverrun(*(timer_t *)si->si_value.sival_ptr);
    printf("  [Timer] Tick %d (overrun=%d)\n", timer_count, overrun);
}

void demo_posix_timer(void) {
    printf("\n--- Demo 3: POSIX Timer (timer_create) ---\n");

    /* Install signal handler */
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGRTMIN, &sa, NULL);

    /* Create timer */
    timer_t timerid;
    struct sigevent sev = {
        .sigev_notify = SIGEV_SIGNAL,
        .sigev_signo  = SIGRTMIN,
        .sigev_value.sival_ptr = &timerid,
    };
    timer_create(CLOCK_MONOTONIC, &sev, &timerid);

    /* Start periodic timer: 200ms interval */
    struct itimerspec its = {
        .it_value    = {.tv_sec = 0, .tv_nsec = 200000000},  /* Initial */
        .it_interval = {.tv_sec = 0, .tv_nsec = 200000000},  /* Periodic */
    };
    timer_settime(timerid, 0, &its, NULL);
    printf("  Started 200ms periodic timer. Waiting for 5 ticks...\n");

    while (timer_count < 5) {
        pause();  /* Wait for signal */
    }

    /* Stop and delete timer */
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    timer_settime(timerid, 0, &its, NULL);
    timer_delete(timerid);
    printf("  Timer stopped after %d ticks.\n", timer_count);
}

/* ============================================================
 * Demo 4: timerfd — file-descriptor-based timer
 * ============================================================ */
void demo_timerfd(void) {
    printf("\n--- Demo 4: timerfd ---\n");

    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd < 0) { perror("timerfd_create"); return; }

    /* Set 100ms periodic timer */
    struct itimerspec its = {
        .it_value    = {.tv_sec = 0, .tv_nsec = 100000000},
        .it_interval = {.tv_sec = 0, .tv_nsec = 100000000},
    };
    timerfd_settime(tfd, 0, &its, NULL);
    printf("  Started 100ms timerfd. Reading 8 ticks...\n");

    struct pollfd pfd = {.fd = tfd, .events = POLLIN};
    int count = 0;

    while (count < 8) {
        int ret = poll(&pfd, 1, 500);
        if (ret > 0 && (pfd.revents & POLLIN)) {
            uint64_t expirations;
            read(tfd, &expirations, sizeof(expirations));
            count += expirations;
            printf("  [timerfd] Tick %d (expirations=%lu)\n",
                   count, (unsigned long)expirations);
        }
    }

    /* Stop timer */
    memset(&its, 0, sizeof(its));
    timerfd_settime(tfd, 0, &its, NULL);
    close(tfd);
    printf("  timerfd stopped.\n");
}

/* ============================================================
 * Demo 5: One-shot timer
 * ============================================================ */
void demo_oneshot(void) {
    printf("\n--- Demo 5: One-Shot Timer ---\n");

    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);

    /* One-shot: fires after 300ms, no repeat */
    struct itimerspec its = {
        .it_value    = {.tv_sec = 0, .tv_nsec = 300000000},
        .it_interval = {.tv_sec = 0, .tv_nsec = 0},  /* No repeat */
    };

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    timerfd_settime(tfd, 0, &its, NULL);
    printf("  One-shot timer set for 300ms...\n");

    uint64_t exp;
    read(tfd, &exp, sizeof(exp));  /* Blocks until timer fires */

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("  Timer fired after %.2f ms (expected ~300ms)\n", elapsed);

    close(tfd);
}

int main(void) {
    printf("=== Timer APIs Demo ===\n\n");

    demo_clocks();
    demo_nanosleep();
    demo_posix_timer();
    demo_timerfd();
    demo_oneshot();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
