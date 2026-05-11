/*
 * fd_passing.c — File descriptor passing via Unix domain sockets (SCM_RIGHTS)
 *
 * Demonstrates passing an open file descriptor between processes.
 *
 * Build: make
 * Run:   ./fd_passing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>

/* Send a file descriptor over a Unix domain socket */
static int send_fd(int sockfd, int fd_to_send)
{
    struct msghdr msg = {0};
    struct iovec iov;
    char dummy = 'F';
    char buf[CMSG_SPACE(sizeof(int))];

    iov.iov_base = &dummy;
    iov.iov_len = 1;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));

    return sendmsg(sockfd, &msg, 0) >= 0 ? 0 : -1;
}

/* Receive a file descriptor from a Unix domain socket */
static int recv_fd(int sockfd)
{
    struct msghdr msg = {0};
    struct iovec iov;
    char dummy;
    char buf[CMSG_SPACE(sizeof(int))];

    iov.iov_base = &dummy;
    iov.iov_len = 1;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if (recvmsg(sockfd, &msg, 0) < 0) return -1;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg == NULL || cmsg->cmsg_type != SCM_RIGHTS) return -1;

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

int main(void)
{
    int sv[2]; /* socketpair */

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* Child: receive a file descriptor from parent */
        close(sv[0]);
        printf("[Child] Waiting to receive file descriptor...\n");

        int received_fd = recv_fd(sv[1]);
        if (received_fd < 0) {
            perror("[Child] recv_fd");
            close(sv[1]);
            exit(1);
        }

        printf("[Child] Received fd=%d, reading content:\n", received_fd);

        char buf[256];
        ssize_t n;
        while ((n = read(received_fd, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("  %s", buf);
        }
        printf("\n[Child] Done.\n");

        close(received_fd);
        close(sv[1]);
        exit(0);
    }

    /* Parent: open a file and send its fd to child */
    close(sv[1]);

    /* Create a temp file */
    const char *tmppath = "/tmp/fd_passing_test.txt";
    int file_fd = open(tmppath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0) { perror("open"); close(sv[0]); return 1; }

    const char *content = "Hello from parent process!\n"
                          "This file descriptor was passed via SCM_RIGHTS.\n";
    write(file_fd, content, strlen(content));
    close(file_fd);

    /* Reopen for reading */
    file_fd = open(tmppath, O_RDONLY);
    if (file_fd < 0) { perror("open read"); close(sv[0]); return 1; }

    printf("[Parent] Sending fd=%d to child...\n", file_fd);
    if (send_fd(sv[0], file_fd) < 0) {
        perror("[Parent] send_fd");
    } else {
        printf("[Parent] File descriptor sent successfully.\n");
    }

    close(file_fd);
    close(sv[0]);

    wait(NULL);
    unlink(tmppath);
    return 0;
}
