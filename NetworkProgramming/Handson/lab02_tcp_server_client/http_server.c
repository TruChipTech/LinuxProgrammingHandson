/*
 * http_server.c — Minimal HTTP/1.0 server demonstrating TCP protocol usage
 *
 * Build: make
 * Run:   ./http_server 8080
 * Test:  curl http://127.0.0.1:8080/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile int running = 1;
static void sigint_handler(int sig) { (void)sig; running = 0; }

static void handle_request(int fd)
{
    char request[4096];
    ssize_t n = read(fd, request, sizeof(request) - 1);
    if (n <= 0) return;
    request[n] = '\0';

    /* Parse first line: METHOD PATH HTTP/VERSION */
    char method[16], path[256], version[16];
    if (sscanf(request, "%15s %255s %15s", method, path, version) != 3) {
        return;
    }

    printf("  %s %s %s\n", method, path, version);

    /* Only handle GET */
    if (strcmp(method, "GET") != 0) {
        const char *resp = "HTTP/1.0 405 Method Not Allowed\r\n"
                           "Content-Length: 22\r\n\r\n"
                           "405 Method Not Allowed";
        write(fd, resp, strlen(resp));
        return;
    }

    /* Build response body */
    char body[4096];
    int body_len;

    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        body_len = snprintf(body, sizeof(body),
            "<html>\n"
            "<head><title>Mini HTTP Server</title></head>\n"
            "<body>\n"
            "<h1>Hello from Mini HTTP Server!</h1>\n"
            "<p>This is a minimal HTTP/1.0 server written in C.</p>\n"
            "<ul>\n"
            "  <li><a href=\"/info\">Server Info</a></li>\n"
            "  <li><a href=\"/time\">Current Time</a></li>\n"
            "</ul>\n"
            "</body>\n"
            "</html>\n");
    } else if (strcmp(path, "/info") == 0) {
        body_len = snprintf(body, sizeof(body),
            "<html><body>\n"
            "<h1>Server Info</h1>\n"
            "<p>HTTP/1.0 server in C</p>\n"
            "<p>PID: %d</p>\n"
            "<p><a href=\"/\">Home</a></p>\n"
            "</body></html>\n", getpid());
    } else if (strcmp(path, "/time") == 0) {
        time_t now = time(NULL);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        body_len = snprintf(body, sizeof(body),
            "<html><body>\n"
            "<h1>Server Time</h1>\n"
            "<p>%s</p>\n"
            "<p><a href=\"/\">Home</a></p>\n"
            "</body></html>\n", timebuf);
    } else {
        /* 404 */
        const char *not_found_body = "<html><body><h1>404 Not Found</h1></body></html>\n";
        char resp[512];
        int resp_len = snprintf(resp, sizeof(resp),
            "HTTP/1.0 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "\r\n%s",
            strlen(not_found_body), not_found_body);
        write(fd, resp, resp_len);
        return;
    }

    /* Send HTTP response */
    char header[512];
    int hdr_len = snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n", body_len);

    write(fd, header, hdr_len);
    write(fd, body, body_len);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); return 1;
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen"); close(server_fd); return 1;
    }

    printf("HTTP Server running on http://0.0.0.0:%d/\n", port);
    printf("Press Ctrl+C to stop.\n\n");

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (!running) break;
            continue;
        }

        handle_request(client_fd);
        close(client_fd);
    }

    close(server_fd);
    printf("\nHTTP Server stopped.\n");
    return 0;
}
