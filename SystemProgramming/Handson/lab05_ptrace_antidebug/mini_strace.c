/*
 * mini_strace.c — A minimal system call tracer using ptrace
 *
 * Build: gcc -Wall -g mini_strace.c -o mini_strace
 * Usage: ./mini_strace /bin/ls
 *        ./mini_strace /bin/echo "Hello"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <errno.h>

/* x86_64 syscall number to name mapping (common ones) */
static const char *syscall_name(long num) {
    switch (num) {
        case 0:   return "read";
        case 1:   return "write";
        case 2:   return "open";
        case 3:   return "close";
        case 4:   return "stat";
        case 5:   return "fstat";
        case 9:   return "mmap";
        case 10:  return "mprotect";
        case 11:  return "munmap";
        case 12:  return "brk";
        case 21:  return "access";
        case 25:  return "mremap";
        case 39:  return "getpid";
        case 56:  return "clone";
        case 57:  return "fork";
        case 59:  return "execve";
        case 60:  return "exit";
        case 63:  return "uname";
        case 72:  return "fcntl";
        case 78:  return "getdents";
        case 79:  return "getcwd";
        case 89:  return "readlink";
        case 96:  return "gettimeofday";
        case 102: return "getuid";
        case 104: return "getgid";
        case 110: return "getppid";
        case 158: return "arch_prctl";
        case 218: return "set_tid_address";
        case 231: return "exit_group";
        case 257: return "openat";
        case 262: return "newfstatat";
        case 302: return "prlimit64";
        case 318: return "getrandom";
        case 332: return "statx";
        default:  return NULL;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    pid_t child = fork();

    if (child == 0) {
        /* Child: request tracing, then exec the target */
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(1);
    }

    /* Parent: trace the child */
    int status;
    int syscall_count = 0;
    int in_syscall = 0;

    waitpid(child, &status, 0);  /* Wait for initial stop */

    printf("[mini_strace] Tracing PID %d: %s\n", child, argv[1]);
    printf("%-6s %-20s %s\n", "COUNT", "SYSCALL", "RETURN");
    printf("------ -------------------- --------\n");

    while (1) {
        /* Continue until next syscall entry/exit */
        if (ptrace(PTRACE_SYSCALL, child, NULL, NULL) < 0) {
            break;
        }

        waitpid(child, &status, 0);

        if (WIFEXITED(status)) {
            printf("\n[mini_strace] Process exited with code %d\n",
                   WEXITSTATUS(status));
            break;
        }

        if (WIFSIGNALED(status)) {
            printf("\n[mini_strace] Process killed by signal %d\n",
                   WTERMSIG(status));
            break;
        }

        /* Read registers to get syscall number and return value */
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, child, NULL, &regs) < 0) {
            break;
        }

        if (!in_syscall) {
            /* Syscall entry */
            syscall_count++;
            const char *name = syscall_name(regs.orig_rax);
            if (name) {
                printf("%-6d %-20s ", syscall_count, name);
            } else {
                printf("%-6d syscall_%-12ld ", syscall_count, regs.orig_rax);
            }
            in_syscall = 1;
        } else {
            /* Syscall exit — print return value */
            long ret = (long)regs.rax;
            if (ret < 0) {
                printf("= -1 (%s)\n", strerror(-(int)ret));
            } else {
                printf("= %ld\n", ret);
            }
            in_syscall = 0;
        }
    }

    printf("\n[mini_strace] Total syscalls traced: %d\n", syscall_count);
    return 0;
}
