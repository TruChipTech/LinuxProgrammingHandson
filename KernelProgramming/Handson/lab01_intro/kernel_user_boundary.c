/*
 * kernel_user_boundary.c — Kernel vs User Space Boundary Demo
 *
 * Demonstrates address space layout and syscall overhead.
 * Build: gcc -Wall -g kernel_user_boundary.c -o kernel_user_boundary
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <time.h>

/* Global variable — lives in data segment */
int global_var = 42;

/* Uninitialized global — lives in BSS */
int bss_var;

void show_address_layout(void) {
    int stack_var = 99;
    void *heap_ptr = malloc(1024);
    
    printf("=== User Space Address Layout ===\n\n");
    printf("  Code  (text):  %p  (main function)\n", (void *)main);
    printf("  Global (data): %p  (global_var = %d)\n", (void *)&global_var, global_var);
    printf("  BSS:           %p  (bss_var = %d)\n", (void *)&bss_var, bss_var);
    printf("  Heap:          %p  (malloc'd)\n", heap_ptr);
    printf("  Stack:         %p  (local var)\n", (void *)&stack_var);
    printf("  Library:       %p  (printf)\n", (void *)printf);
    
    printf("\n  Observations:\n");
    printf("  - Code is at low addresses\n");
    printf("  - Stack is at high addresses (grows down)\n");
    printf("  - Heap grows up from data segment\n");
    printf("  - All addresses are in user space (< 0x7fff...)\n");
    
    free(heap_ptr);
}

void measure_syscall_overhead(void) {
    printf("\n=== System Call Overhead ===\n\n");
    
    int iterations = 1000000;
    struct timespec start, end;
    
    /* Measure function call overhead */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        (void)global_var;  /* Simple memory access */
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double func_ns = ((end.tv_sec - start.tv_sec) * 1e9 + 
                      (end.tv_nsec - start.tv_nsec)) / iterations;
    
    /* Measure getpid() syscall overhead */
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        getpid();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double syscall_ns = ((end.tv_sec - start.tv_sec) * 1e9 + 
                         (end.tv_nsec - start.tv_nsec)) / iterations;
    
    printf("  Memory access:   %.1f ns/op\n", func_ns);
    printf("  getpid() syscall: %.1f ns/op\n", syscall_ns);
    printf("  Overhead ratio:   %.1fx\n", syscall_ns / func_ns);
    printf("  (getpid is one of the cheapest syscalls)\n");
}

void show_kernel_info(void) {
    printf("\n=== Kernel Information (via syscalls) ===\n\n");
    
    /* uname */
    struct utsname uts;
    uname(&uts);
    printf("  Kernel:    %s %s\n", uts.sysname, uts.release);
    printf("  Node:      %s\n", uts.nodename);
    printf("  Machine:   %s\n", uts.machine);
    
    /* sysinfo */
    struct sysinfo si;
    sysinfo(&si);
    printf("  Uptime:    %ld hours, %ld minutes\n", 
           si.uptime / 3600, (si.uptime % 3600) / 60);
    printf("  RAM:       %lu MB total, %lu MB free\n",
           si.totalram * si.mem_unit / (1024 * 1024),
           si.freeram * si.mem_unit / (1024 * 1024));
    printf("  Procs:     %d\n", si.procs);
    
    /* PID */
    printf("  My PID:    %d\n", getpid());
    printf("  Parent:    %d\n", getppid());
    printf("  UID:       %d\n", getuid());
}

void show_proc_maps(void) {
    printf("\n=== /proc/self/maps (first 10 entries) ===\n\n");
    
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) { perror("  Cannot open /proc/self/maps"); return; }
    
    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < 10) {
        printf("  %s", line);
        count++;
    }
    fclose(fp);
}

int main(void) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Kernel vs User Space Boundary Demo     ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");
    
    show_address_layout();
    show_kernel_info();
    measure_syscall_overhead();
    show_proc_maps();
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}
