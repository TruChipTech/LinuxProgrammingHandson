/*
 * custom_init.c — Minimal Init Program (PID 1)
 *
 * Build: gcc -Wall -static custom_init.c -o custom_init
 * Usage: Boot with kernel parameter init=/custom_init
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <signal.h>

static void mount_virtual_fs(void) {
    printf("[init] Mounting virtual filesystems...\n");
    mount("proc",  "/proc", "proc",  0, NULL);
    mount("sysfs", "/sys",  "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    printf("[init] /proc, /sys, /dev mounted.\n");
}

static void print_system_info(void) {
    struct utsname uts;
    struct sysinfo si;
    
    uname(&uts);
    sysinfo(&si);
    
    printf("\n");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║          Custom Init (PID %d)             ║\n", getpid());
    printf("╠══════════════════════════════════════════╣\n");
    printf("║ Kernel:  %-30s ║\n", uts.release);
    printf("║ Machine: %-30s ║\n", uts.machine);
    printf("║ Node:    %-30s ║\n", uts.nodename);
    printf("║ RAM:     %-3lu MB total                     ║\n",
           si.totalram * si.mem_unit / (1024*1024));
    printf("╚══════════════════════════════════════════╝\n");
    printf("\n");
}

static void spawn_shell(void) {
    printf("[init] Spawning shell...\n\n");
    
    pid_t pid = fork();
    if (pid == 0) {
        /* Child — exec shell */
        char *argv[] = {"/bin/sh", NULL};
        char *envp[] = {
            "HOME=/",
            "PATH=/bin:/sbin:/usr/bin:/usr/sbin",
            "TERM=linux",
            NULL
        };
        execve("/bin/sh", argv, envp);
        perror("[init] execve failed");
        _exit(1);
    } else if (pid > 0) {
        /* Parent — wait for shell */
        int status;
        waitpid(pid, &status, 0);
        printf("[init] Shell exited with status %d\n", WEXITSTATUS(status));
    } else {
        perror("[init] fork failed");
    }
}

static void sig_handler(int sig) {
    printf("[init] Received signal %d\n", sig);
}

int main(void) {
    /* PID 1 must not die — set up signal handling */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    printf("[init] Custom init starting (PID %d)...\n", getpid());
    
    if (getpid() != 1) {
        printf("[init] WARNING: Not running as PID 1 (PID=%d)\n", getpid());
        printf("[init] This program is designed to be the init process.\n");
        printf("[init] Boot with: init=/path/to/custom_init\n");
    }
    
    mount_virtual_fs();
    print_system_info();
    
    /* Respawn shell if it exits */
    while (1) {
        spawn_shell();
        printf("[init] Respawning shell in 2 seconds...\n");
        sleep(2);
    }
    
    return 0;
}
