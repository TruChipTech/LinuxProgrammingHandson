/*
 * mqueue_demo.c — POSIX Message Queue Demo
 *
 * Build: gcc -Wall -g mqueue_demo.c -o mqueue_demo -lrt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <errno.h>
#include <sys/wait.h>

#define QUEUE_NAME  "/demo_queue"
#define MAX_MSG_SIZE 256
#define MAX_MSGS     10

int main(void) {
    printf("=== POSIX Message Queue Demo ===\n\n");

    /* Remove any existing queue */
    mq_unlink(QUEUE_NAME);

    /* Create the queue */
    struct mq_attr attr = {
        .mq_flags   = 0,
        .mq_maxmsg  = MAX_MSGS,
        .mq_msgsize = MAX_MSG_SIZE,
        .mq_curmsgs = 0,
    };

    mqd_t mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    printf("[Main] Queue '%s' created.\n\n", QUEUE_NAME);

    pid_t pid = fork();
    if (pid == 0) {
        /* Child: Producer — send messages with priorities */
        mqd_t child_mq = mq_open(QUEUE_NAME, O_WRONLY);

        struct {
            const char *msg;
            unsigned int priority;
        } messages[] = {
            {"Low priority message",    1},
            {"Normal message",          5},
            {"URGENT: System alert!",  10},
            {"Another normal message",  5},
            {"Background task done",    2},
        };

        int n = sizeof(messages) / sizeof(messages[0]);
        for (int i = 0; i < n; i++) {
            mq_send(child_mq, messages[i].msg,
                    strlen(messages[i].msg) + 1, messages[i].priority);
            printf("  [Producer] Sent (pri=%2u): '%s'\n",
                   messages[i].priority, messages[i].msg);
        }

        mq_close(child_mq);
        _exit(0);
    }

    /* Parent: Consumer — wait then receive (highest priority first) */
    waitpid(pid, NULL, 0);

    printf("\n[Consumer] Receiving messages (highest priority first):\n");

    struct mq_attr current_attr;
    mq_getattr(mq, &current_attr);
    printf("[Consumer] Messages in queue: %ld\n\n", current_attr.mq_curmsgs);

    char buf[MAX_MSG_SIZE + 1];
    unsigned int priority;

    while (current_attr.mq_curmsgs > 0) {
        ssize_t n = mq_receive(mq, buf, MAX_MSG_SIZE + 1, &priority);
        if (n < 0) { perror("mq_receive"); break; }
        buf[n] = '\0';
        printf("  [Consumer] Received (pri=%2u): '%s'\n", priority, buf);
        mq_getattr(mq, &current_attr);
    }

    mq_close(mq);
    mq_unlink(QUEUE_NAME);

    printf("\n=== Demo Complete ===\n");
    return 0;
}
