/*
 * i2c_scan.c — Userspace I2C Bus Scanner
 *
 * Compile: gcc -Wall -o i2c_scan i2c_scan.c
 * Run:     sudo ./i2c_scan /dev/i2c-1
 *
 * Scans all addresses 0x03–0x77 like i2cdetect.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int main(int argc, char *argv[])
{
    const char *bus = argc > 1 ? argv[1] : "/dev/i2c-1";
    int fd;
    unsigned char dummy;

    fd = open(bus, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("I2C Bus Scanner — %s\n", bus);
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");

    for (int addr = 0; addr < 0x80; addr++) {
        if (addr % 16 == 0)
            printf("%02x: ", addr);

        if (addr < 0x03 || addr > 0x77) {
            printf("   ");
        } else {
            if (ioctl(fd, I2C_SLAVE, addr) < 0) {
                printf("-- ");
            } else {
                if (read(fd, &dummy, 1) == 1)
                    printf("%02x ", addr);
                else
                    printf("-- ");
            }
        }

        if (addr % 16 == 15)
            printf("\n");
    }

    close(fd);
    return 0;
}
