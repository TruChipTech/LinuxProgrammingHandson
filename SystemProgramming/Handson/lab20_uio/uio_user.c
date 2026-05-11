/*
 * uio_user.c — Userspace UIO Driver Demo
 *
 * Demonstrates accessing a UIO device from userspace:
 *   - Open /dev/uioN
 *   - mmap device memory
 *   - Wait for interrupts via read()
 *
 * Build: gcc -Wall -g uio_user.c -o uio_user
 * Usage: sudo ./uio_user /dev/uio0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>

#define UIO_MAP_SIZE 4096

/* Read a UIO sysfs attribute */
static int read_sysfs_attr(const char *uio_name, const char *attr, char *buf, size_t len) {
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/uio/%s/%s", uio_name, attr);

    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    ssize_t n = read(fd, buf, len - 1);
    close(fd);
    if (n < 0) return -1;
    buf[n] = '\0';
    /* Remove trailing newline */
    if (n > 0 && buf[n-1] == '\n') buf[n-1] = '\0';
    return 0;
}

/* Print UIO device info from sysfs */
static void print_uio_info(const char *uio_name) {
    char buf[256];

    printf("=== UIO Device Info ===\n");
    printf("  Device: %s\n", uio_name);

    if (read_sysfs_attr(uio_name, "name", buf, sizeof(buf)) == 0)
        printf("  Name:    %s\n", buf);
    if (read_sysfs_attr(uio_name, "version", buf, sizeof(buf)) == 0)
        printf("  Version: %s\n", buf);

    /* Map info */
    if (read_sysfs_attr(uio_name, "maps/map0/addr", buf, sizeof(buf)) == 0)
        printf("  Map0 addr: %s\n", buf);
    if (read_sysfs_attr(uio_name, "maps/map0/size", buf, sizeof(buf)) == 0)
        printf("  Map0 size: %s\n", buf);
}

int main(int argc, char *argv[]) {
    const char *dev_path = argc > 1 ? argv[1] : "/dev/uio0";

    /* Extract uio name (e.g., "uio0" from "/dev/uio0") */
    const char *uio_name = strrchr(dev_path, '/');
    uio_name = uio_name ? uio_name + 1 : dev_path;

    print_uio_info(uio_name);

    /* Open UIO device */
    int fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s: %s\n", dev_path, strerror(errno));
        fprintf(stderr, "Make sure a UIO kernel module is loaded.\n");
        return 1;
    }
    printf("\n  Opened %s (fd=%d)\n", dev_path, fd);

    /* Memory-map the device registers */
    void *map = mmap(NULL, UIO_MAP_SIZE, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        printf("  (mmap may fail if the device has no mappable memory)\n");
    } else {
        printf("  Mapped device memory at %p (%d bytes)\n", map, UIO_MAP_SIZE);

        /* Read first few 32-bit registers */
        volatile uint32_t *regs = (volatile uint32_t *)map;
        printf("  Register dump (first 4 words):\n");
        for (int i = 0; i < 4; i++) {
            printf("    [%02X] = 0x%08X\n", i * 4, regs[i]);
        }

        munmap(map, UIO_MAP_SIZE);
    }

    /* Wait for interrupt */
    printf("\n  Waiting for interrupt (Ctrl+C to abort)...\n");
    uint32_t irq_count;
    ssize_t n = read(fd, &irq_count, sizeof(irq_count));
    if (n == sizeof(irq_count)) {
        printf("  Interrupt received! Count: %u\n", irq_count);
    } else {
        printf("  read() returned %zd (may need interrupt from device)\n", n);
    }

    /* Re-enable interrupt */
    uint32_t enable = 1;
    write(fd, &enable, sizeof(enable));

    close(fd);
    printf("\n=== UIO Demo Complete ===\n");
    return 0;
}
