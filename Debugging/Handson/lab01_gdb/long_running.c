/*
 * long_running.c — Long-running program for live-attach debugging
 *
 * Runs a loop with periodic work. Attach GDB while running:
 *   ./long_running &
 *   sudo gdb -p $(pgrep long_running)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static int counter = 0;
static int suspicious_value = 0;

void do_work(int iteration)
{
    counter += iteration;
    if (iteration % 100 == 0) {
        /* This function occasionally does something "wrong" */
        suspicious_value = counter * -1;
    }
}

int main(void)
{
    int i = 0;

    printf("Long-running process started (PID=%d)\n", getpid());
    printf("Attach GDB: sudo gdb -p %d\n", getpid());

    while (1) {
        do_work(i);

        if (i % 500 == 0)
            printf("[%d] counter=%d suspicious=%d\n",
                   i, counter, suspicious_value);

        usleep(10000);  /* 10ms */
        i++;
    }

    return 0;
}
