/*
 * unix_socket_demo.c — Unix Domain Socket Server/Client in one process
 *
 * Build: gcc -Wall -g unix_socket_demo.c -o unix_socket_demo -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/demo_unix.sock"
#define MAX_MSG 256

/* ============================================================
 * Server thread
 * ============================================================ */
void *server_func(void *arg) {
    (void)arg;

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return NULL; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return NULL;
    }

    listen(server_fd, 5);
    printf("[Server] Listening on %s\n", SOCKET_PATH);

    /* Accept 3 clients */
    for (int i = 0; i < 3; i++) {
        int client = accept(server_fd, NULL, NULL);
        if (client < 0) { perror("accept"); continue; }

        char buf[MAX_MSG];
        ssize_t n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            printf("[Server] Client %d says: '%s'\n", i, buf);

            /* Echo back */
            char reply[MAX_MSG];
            snprintf(reply, sizeof(reply), "ACK: %s", buf);
            send(client, reply, strlen(reply), 0);
        }
        close(client);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return NULL;
}

/* ============================================================
 * Client function
 * ============================================================ */
void client_send(int id, const char *message) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(fd); return;
    }

    send(fd, message, strlen(message), 0);

    char reply[MAX_MSG];
    ssize_t n = recv(fd, reply, sizeof(reply) - 1, 0);
    if (n > 0) {
        reply[n] = '\0';
        printf("[Client %d] Reply: '%s'\n", id, reply);
    }

    close(fd);
}

/* ============================================================
 * Demo: File descriptor passing (SCM_RIGHTS)
 * ============================================================ */
void demo_fd_passing(void) {
    printf("\n--- File Descriptor Passing (SCM_RIGHTS) ---\n");

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair"); return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* Child: open a file and send the fd to parent */
        close(sv[0]);

        int fd = open("/etc/hostname", 0);
        if (fd < 0) { perror("open"); _exit(1); }

        /* Prepare ancillary data */
        char buf[1] = {'F'};
        struct iovec iov = { .iov_base = buf, .iov_len = 1 };

        char cmsg_buf[CMSG_SPACE(sizeof(int))];
        struct msghdr msg = {
            .msg_iov = &iov, .msg_iovlen = 1,
            .msg_control = cmsg_buf, .msg_controllen = sizeof(cmsg_buf),
        };

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));

        sendmsg(sv[1], &msg, 0);
        printf("  [Child]  Sent fd %d to parent\n", fd);
        close(fd);
        close(sv[1]);
        _exit(0);
    }

    /* Parent: receive the fd */
    close(sv[1]);

    char buf[1];
    struct iovec iov = { .iov_base = buf, .iov_len = 1 };
    char cmsg_buf[CMSG_SPACE(sizeof(int))];
    struct msghdr msg = {
        .msg_iov = &iov, .msg_iovlen = 1,
        .msg_control = cmsg_buf, .msg_controllen = sizeof(cmsg_buf),
    };

    recvmsg(sv[0], &msg, 0);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    int received_fd;
    memcpy(&received_fd, CMSG_DATA(cmsg), sizeof(int));
    printf("  [Parent] Received fd %d\n", received_fd);

    /* Read from the received fd */
    char content[256];
    ssize_t n = read(received_fd, content, sizeof(content) - 1);
    if (n > 0) {
        content[n] = '\0';
        printf("  [Parent] Read from received fd: '%s'\n", content);
    }

    close(received_fd);
    close(sv[0]);
    waitpid(pid, NULL, 0);
}

int main(void) {
    printf("=== Unix Domain Socket Demo ===\n\n");

    /* Start server thread */
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_func, NULL);
    usleep(100000);  /* Let server start */

    /* Send messages from "clients" */
    client_send(0, "Hello from client 0");
    client_send(1, "Greetings from client 1");
    client_send(2, "Bye from client 2");

    pthread_join(server_thread, NULL);

    /* File descriptor passing demo */
    demo_fd_passing();

    printf("\n=== Demo Complete ===\n");
    return 0;
}
