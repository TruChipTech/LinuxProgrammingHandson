/*
 * pipe_demo.c — Anonymous Pipes and Bidirectional Communication
 *
 * Build: gcc -Wall -g pipe_demo.c -o pipe_demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

/* ============================================================
 * Demo 1: Basic pipe — parent writes, child reads
 * ============================================================ */
void demo_basic_pipe(void) {
    printf("--- Demo 1: Basic Pipe ---\n");

    int pipefd[2];  /* pipefd[0]=read, pipefd[1]=write */
    if (pipe(pipefd) < 0) { perror("pipe"); return; }

    pid_t pid = fork();
    if (pid == 0) {
        /* Child: reads from pipe */
        close(pipefd[1]);  /* Close write end */
        char buf[256];
        ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
        buf[n] = '\0';
        printf("  [Child]  Received: '%s'\n", buf);
        close(pipefd[0]);
        _exit(0);
    }

    /* Parent: writes to pipe */
    close(pipefd[0]);  /* Close read end */
    const char *msg = "Hello from parent via pipe!";
    write(pipefd[1], msg, strlen(msg));
    printf("  [Parent] Sent: '%s'\n", msg);
    close(pipefd[1]);
    waitpid(pid, NULL, 0);
}

/* ============================================================
 * Demo 2: Bidirectional communication (two pipes)
 * ============================================================ */
void demo_bidirectional(void) {
    printf("\n--- Demo 2: Bidirectional Pipes ---\n");

    int parent_to_child[2], child_to_parent[2];
    pipe(parent_to_child);
    pipe(child_to_parent);

    pid_t pid = fork();
    if (pid == 0) {
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        char buf[256];
        ssize_t n = read(parent_to_child[0], buf, sizeof(buf) - 1);
        buf[n] = '\0';
        printf("  [Child]  Got: '%s'\n", buf);

        const char *reply = "ACK from child!";
        write(child_to_parent[1], reply, strlen(reply));

        close(parent_to_child[0]);
        close(child_to_parent[1]);
        _exit(0);
    }

    close(parent_to_child[0]);
    close(child_to_parent[1]);

    const char *msg = "Request from parent";
    write(parent_to_child[1], msg, strlen(msg));
    close(parent_to_child[1]);

    char reply[256];
    ssize_t n = read(child_to_parent[0], reply, sizeof(reply) - 1);
    reply[n] = '\0';
    printf("  [Parent] Reply: '%s'\n", reply);

    close(child_to_parent[0]);
    waitpid(pid, NULL, 0);
}

/* ============================================================
 * Demo 3: dup2 — redirect stdout through pipe
 * ============================================================ */
void demo_dup2_redirect(void) {
    printf("\n--- Demo 3: dup2 Redirect ---\n");

    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);  /* stdout → pipe */
        close(pipefd[1]);

        execlp("uname", "uname", "-a", NULL);
        _exit(1);
    }

    close(pipefd[1]);
    char buf[512];
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    buf[n] = '\0';
    printf("  [Parent] Captured child stdout:\n  %s", buf);

    close(pipefd[0]);
    waitpid(pid, NULL, 0);
}

/* ============================================================
 * Demo 4: Non-blocking pipe read
 * ============================================================ */
void demo_nonblocking(void) {
    printf("\n--- Demo 4: Non-Blocking Pipe ---\n");

    int pipefd[2];
    pipe(pipefd);

    /* Set read end to non-blocking */
    int flags = fcntl(pipefd[0], F_GETFL);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    char buf[256];
    ssize_t n = read(pipefd[0], buf, sizeof(buf));
    if (n < 0 && errno == EAGAIN) {
        printf("  Non-blocking read: EAGAIN (no data yet) — correct!\n");
    }

    /* Now write something and read again */
    write(pipefd[1], "data", 4);
    n = read(pipefd[0], buf, sizeof(buf));
    if (n > 0) {
        buf[n] = '\0';
        printf("  Non-blocking read: got '%s'\n", buf);
    }

    close(pipefd[0]);
    close(pipefd[1]);
}

/* ============================================================
 * Demo 5: Pipe pipeline (like "ls | wc -l" in shell)
 * ============================================================ */
void demo_pipe_pipeline(void) {
    printf("\n--- Demo 5: Pipe Pipeline (ls | wc -l) ---\n");

    int pipefd[2];
    pipe(pipefd);

    pid_t p1 = fork();
    if (p1 == 0) {
        /* First child: ls */
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execlp("ls", "ls", "/usr/bin", NULL);
        _exit(1);
    }

    pid_t p2 = fork();
    if (p2 == 0) {
        /* Second child: wc -l */
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execlp("wc", "wc", "-l", NULL);
        _exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);
}

int main(void) {
    printf("=== Pipe Communication Demo ===\n\n");

    demo_basic_pipe();
    demo_bidirectional();
    demo_dup2_redirect();
    demo_nonblocking();
    demo_pipe_pipeline();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
