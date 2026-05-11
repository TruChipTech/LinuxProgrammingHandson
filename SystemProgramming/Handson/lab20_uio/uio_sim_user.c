/*
 * uio_sim_user.c — Userspace Driver for Simulated UIO Device
 *
 * Works with the uio_sim kernel module (sim_device/).
 * Demonstrates register read/write and interrupt handling.
 *
 * Build: gcc -Wall -g uio_sim_user.c -o uio_sim_user
 * Usage: sudo ./uio_sim_user /dev/uio0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>

#define MAP_SIZE      4096

/* Simulated device registers (must match kernel module) */
#define REG_ID        0x00   /* Device ID (read-only) */
#define REG_STATUS    0x04   /* Status register */
#define REG_CONTROL   0x08   /* Control register */
#define REG_DATA      0x0C   /* Data register */
#define REG_IRQ_ACK   0x10   /* IRQ acknowledge */

static volatile int running = 1;

void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[]) {
    const char *dev_path = argc > 1 ? argv[1] : "/dev/uio0";

    signal(SIGINT, sigint_handler);

    printf("=== Simulated UIO Device Driver ===\n");

    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s: %s\n", dev_path, strerror(errno));
        fprintf(stderr, "Load the uio_sim kernel module first:\n");
        fprintf(stderr, "  cd sim_device && make && sudo insmod uio_sim.ko\n");
        return 1;
    }

    /* Map device memory */
    volatile uint32_t *regs = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, 0);
    if (regs == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    /* Read device ID */
    printf("  Device ID:     0x%08X\n", regs[REG_ID / 4]);
    printf("  Status:        0x%08X\n", regs[REG_STATUS / 4]);

    /* Write to control register */
    printf("\n  Writing 0x01 to control register (enable device)...\n");
    regs[REG_CONTROL / 4] = 0x01;
    printf("  Control:       0x%08X\n", regs[REG_CONTROL / 4]);

    /* Write data */
    printf("  Writing 0xDEADBEEF to data register...\n");
    regs[REG_DATA / 4] = 0xDEADBEEF;
    printf("  Data readback: 0x%08X\n", regs[REG_DATA / 4]);

    /* Poll for interrupts */
    printf("\n  Polling for interrupts (Ctrl+C to stop)...\n");

    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    int irq_total = 0;

    while (running && irq_total < 10) {
        int ret = poll(&pfd, 1, 1000);  /* 1s timeout */
        if (ret > 0 && (pfd.revents & POLLIN)) {
            uint32_t irq_count;
            if (read(fd, &irq_count, sizeof(irq_count)) == sizeof(irq_count)) {
                irq_total++;
                printf("  IRQ #%d (kernel count: %u)\n", irq_total, irq_count);

                /* Acknowledge interrupt */
                regs[REG_IRQ_ACK / 4] = 1;

                /* Re-enable */
                uint32_t enable = 1;
                write(fd, &enable, sizeof(enable));
            }
        } else if (ret == 0) {
            printf("  (no interrupt in 1s)\n");
        }
    }

    /* Disable device */
    regs[REG_CONTROL / 4] = 0x00;
    printf("\n  Device disabled.\n");

    munmap((void *)regs, MAP_SIZE);
    close(fd);
    printf("=== Done ===\n");
    return 0;
}
