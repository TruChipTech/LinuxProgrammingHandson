/*
 * file_watcher.c — Monitor file/directory changes using inotify
 *
 * Build: gcc -Wall -g file_watcher.c -o file_watcher
 * Usage: ./file_watcher /tmp
 *        ./file_watcher /path/to/watch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <time.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN    (1024 * (EVENT_SIZE + 256))

static const char *event_type_str(uint32_t mask) {
    if (mask & IN_CREATE)      return "CREATED";
    if (mask & IN_DELETE)      return "DELETED";
    if (mask & IN_MODIFY)      return "MODIFIED";
    if (mask & IN_MOVED_FROM)  return "MOVED_FROM";
    if (mask & IN_MOVED_TO)    return "MOVED_TO";
    if (mask & IN_ATTRIB)      return "ATTRIB_CHANGE";
    if (mask & IN_OPEN)        return "OPENED";
    if (mask & IN_CLOSE_WRITE) return "CLOSE_WRITE";
    if (mask & IN_CLOSE_NOWRITE) return "CLOSE_NOWRITE";
    if (mask & IN_ACCESS)      return "ACCESSED";
    return "UNKNOWN";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_watch>\n", argv[0]);
        return 1;
    }

    const char *watch_path = argv[1];

    /* Initialize inotify */
    int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return 1;
    }

    /* Add watch for multiple events */
    uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY |
                    IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB;

    int wd = inotify_add_watch(fd, watch_path, mask);
    if (wd < 0) {
        perror("inotify_add_watch");
        close(fd);
        return 1;
    }

    printf("=== File Watcher ===\n");
    printf("Monitoring: %s\n", watch_path);
    printf("Press Ctrl+C to stop.\n\n");
    printf("%-24s %-14s %-6s %s\n", "TIMESTAMP", "EVENT", "TYPE", "NAME");
    printf("%-24s %-14s %-6s %s\n", "------------------------",
           "--------------", "------", "----");

    char buf[BUF_LEN];
    int event_count = 0;

    while (1) {
        ssize_t len = read(fd, buf, BUF_LEN);
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("read");
            break;
        }

        ssize_t i = 0;
        while (i < len) {
            struct inotify_event *event = (struct inotify_event *)&buf[i];
            event_count++;

            /* Get timestamp */
            time_t now = time(NULL);
            struct tm *tm = localtime(&now);
            char ts[32];
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm);

            const char *type = (event->mask & IN_ISDIR) ? "DIR" : "FILE";
            const char *name = event->len ? event->name : "(watch root)";

            printf("%-24s %-14s %-6s %s\n",
                   ts, event_type_str(event->mask), type, name);

            i += EVENT_SIZE + event->len;
        }
    }

    printf("\nTotal events: %d\n", event_count);

    inotify_rm_watch(fd, wd);
    close(fd);

    return 0;
}
