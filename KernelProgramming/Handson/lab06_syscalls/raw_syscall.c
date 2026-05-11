/*
 * raw_syscall.c — Making Raw System Calls
 *
 * Demonstrates bypassing glibc to invoke syscalls directly.
 * Build: gcc -Wall -g raw_syscall.c -o raw_syscall
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/utsname.h>

/* Syscall numbers for x86_64 */
/* See: /usr/include/asm/unistd_64.h or arch/x86/entry/syscalls/syscall_64.tbl */

void demo_syscall_wrapper(void) {
    printf("=== Method 1: syscall() wrapper ===\n\n");
    
    /* getpid via syscall() */
    long pid = syscall(SYS_getpid);
    printf("  getpid() via syscall(): %ld\n", pid);
    printf("  getpid() via glibc:     %d\n", getpid());
    
    /* write via syscall() */
    const char *msg = "  Hello via raw write syscall!\n";
    syscall(SYS_write, 1, msg, strlen(msg));
    
    /* getuid via syscall() */
    long uid = syscall(SYS_getuid);
    printf("  getuid() via syscall(): %ld\n", uid);
    
    /* uname via syscall() */
    struct utsname uts;
    syscall(SYS_uname, &uts);
    printf("  uname via syscall():    %s %s\n", uts.sysname, uts.release);
}

void demo_inline_asm(void) {
    printf("\n=== Method 2: Inline Assembly (x86_64) ===\n\n");
    
#ifdef __x86_64__
    /* getpid via inline asm */
    long pid;
    __asm__ volatile (
        "mov $39, %%rax\n"     /* __NR_getpid = 39 */
        "syscall\n"
        : "=a" (pid)
        :
        : "rcx", "r11", "memory"
    );
    printf("  getpid() via asm: %ld\n", pid);
    
    /* write via inline asm */
    const char *msg = "  Hello via inline asm syscall!\n";
    long ret;
    __asm__ volatile (
        "mov $1, %%rax\n"      /* __NR_write = 1 */
        "mov $1, %%rdi\n"      /* fd = 1 (stdout) */
        "mov %1, %%rsi\n"      /* buf */
        "mov %2, %%rdx\n"      /* count */
        "syscall\n"
        : "=a" (ret)
        : "r" (msg), "r" ((long)strlen(msg))
        : "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
    printf("  write() returned: %ld bytes\n", ret);
    
    /* getuid via inline asm */
    long uid;
    __asm__ volatile (
        "mov $102, %%rax\n"    /* __NR_getuid = 102 */
        "syscall\n"
        : "=a" (uid)
        :
        : "rcx", "r11", "memory"
    );
    printf("  getuid() via asm: %ld\n", uid);
#else
    printf("  (Inline asm demo only available on x86_64)\n");
#endif
}

void demo_syscall_numbers(void) {
    printf("\n=== Common x86_64 Syscall Numbers ===\n\n");
    
    printf("  %-20s %s\n", "Syscall", "Number");
    printf("  %-20s %s\n", "-------", "------");
    printf("  %-20s %d\n", "read",     SYS_read);
    printf("  %-20s %d\n", "write",    SYS_write);
    printf("  %-20s %d\n", "open",     SYS_open);
    printf("  %-20s %d\n", "close",    SYS_close);
    printf("  %-20s %d\n", "stat",     SYS_stat);
    printf("  %-20s %d\n", "mmap",     SYS_mmap);
    printf("  %-20s %d\n", "brk",      SYS_brk);
    printf("  %-20s %d\n", "fork",     SYS_fork);
    printf("  %-20s %d\n", "execve",   SYS_execve);
    printf("  %-20s %d\n", "exit",     SYS_exit);
    printf("  %-20s %d\n", "getpid",   SYS_getpid);
    printf("  %-20s %d\n", "getuid",   SYS_getuid);
    printf("  %-20s %d\n", "socket",   SYS_socket);
    printf("  %-20s %d\n", "clone",    SYS_clone);
}

int main(void) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║        Raw System Call Demo               ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    
    demo_syscall_wrapper();
    demo_inline_asm();
    demo_syscall_numbers();
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}
