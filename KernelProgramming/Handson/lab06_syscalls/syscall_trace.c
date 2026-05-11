/*
 * syscall_trace.c — System Call Tracing & Measurement
 *
 * Build: gcc -Wall -g syscall_trace.c -o syscall_trace
 * Usage: strace -c ./syscall_trace
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <time.h>

void demo_file_syscalls(void) {
    printf("--- File System Calls ---\n");
    
    /* open, write, close */
    int fd = open("/tmp/syscall_test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        const char *msg = "Hello from syscall demo!\n";
        write(fd, msg, strlen(msg));
        
        /* fsync — flush to disk */
        fsync(fd);
        close(fd);
        printf("  Created /tmp/syscall_test.txt\n");
    }
    
    /* stat */
    struct stat st;
    if (stat("/tmp/syscall_test.txt", &st) == 0) {
        printf("  File size: %ld bytes\n", st.st_size);
        printf("  Inode: %lu\n", st.st_ino);
    }
    
    /* read */
    fd = open("/tmp/syscall_test.txt", O_RDONLY);
    if (fd >= 0) {
        char buf[128];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("  Read back: %s", buf);
        }
        close(fd);
    }
    
    /* unlink */
    unlink("/tmp/syscall_test.txt");
    printf("  Cleaned up test file\n");
}

void demo_process_syscalls(void) {
    printf("\n--- Process System Calls ---\n");
    
    printf("  PID:    %d\n", getpid());
    printf("  PPID:   %d\n", getppid());
    printf("  UID:    %d\n", getuid());
    printf("  EUID:   %d\n", geteuid());
    printf("  GID:    %d\n", getgid());
    
    /* getcwd */
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("  CWD:    %s\n", cwd);
    }
    
    /* uname */
    struct utsname uts;
    uname(&uts);
    printf("  Kernel: %s %s\n", uts.sysname, uts.release);
}

void demo_time_syscalls(void) {
    printf("\n--- Time System Calls ---\n");
    
    /* time */
    time_t now = time(NULL);
    printf("  time():          %ld\n", now);
    
    /* clock_gettime */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("  CLOCK_REALTIME:  %ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("  CLOCK_MONOTONIC: %ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
}

void demo_info_syscalls(void) {
    printf("\n--- System Info Calls ---\n");
    
    /* sysinfo */
    struct sysinfo si;
    sysinfo(&si);
    printf("  Uptime:  %ld seconds\n", si.uptime);
    printf("  RAM:     %lu MB total, %lu MB free\n",
           si.totalram * si.mem_unit / (1024 * 1024),
           si.freeram * si.mem_unit / (1024 * 1024));
    printf("  Procs:   %d\n", si.procs);
}

int main(void) {
    printf("=== System Call Tracing Demo ===\n");
    printf("Run with: strace -c ./syscall_trace\n\n");
    
    demo_file_syscalls();
    demo_process_syscalls();
    demo_time_syscalls();
    demo_info_syscalls();
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}
