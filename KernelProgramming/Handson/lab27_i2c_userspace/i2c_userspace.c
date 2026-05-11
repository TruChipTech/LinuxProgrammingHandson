/*
 * i2c_userspace.c — Userspace I2C Communication
 *
 * Compile: gcc -Wall -o i2c_userspace i2c_userspace.c
 * Run:     sudo ./i2c_userspace /dev/i2c-1 0x68
 *
 * Reads the WHO_AM_I register of an I2C device (e.g., MPU6050).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>

static int i2c_read_reg(int fd, unsigned char reg, unsigned char *val)
{
    if (write(fd, &reg, 1) != 1) {
        perror("write register address");
        return -1;
    }
    if (read(fd, val, 1) != 1) {
        perror("read register value");
        return -1;
    }
    return 0;
}

static int i2c_write_reg(int fd, unsigned char reg, unsigned char val)
{
    unsigned char buf[2] = { reg, val };
    if (write(fd, buf, 2) != 2) {
        perror("write register");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *i2c_bus = "/dev/i2c-1";
    int addr = 0x68;
    int fd;
    unsigned char val;

    if (argc >= 2)
        i2c_bus = argv[1];
    if (argc >= 3)
        addr = (int)strtol(argv[2], NULL, 0);

    printf("=== I2C Userspace Demo ===\n");
    printf("Bus: %s, Device address: 0x%02x\n\n", i2c_bus, addr);

    fd = open(i2c_bus, O_RDWR);
    if (fd < 0) {
        perror("open I2C bus");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        perror("ioctl I2C_SLAVE");
        close(fd);
        return 1;
    }

    /* Read WHO_AM_I (reg 0x75 for MPU6050) */
    printf("1. Reading WHO_AM_I (reg 0x75)...\n");
    if (i2c_read_reg(fd, 0x75, &val) == 0)
        printf("   WHO_AM_I = 0x%02x\n\n", val);

    /* Read a few more registers */
    printf("2. Reading registers 0x00-0x07:\n");
    for (int reg = 0x00; reg <= 0x07; reg++) {
        if (i2c_read_reg(fd, reg, &val) == 0)
            printf("   Reg 0x%02x = 0x%02x\n", reg, val);
    }
    printf("\n");

    /* Write example: wake up MPU6050 (reg 0x6B = 0x00) */
    printf("3. Writing 0x00 to reg 0x6B (wake up)...\n");
    if (i2c_write_reg(fd, 0x6B, 0x00) == 0)
        printf("   Write successful\n\n");

    /* Read accelerometer data (registers 0x3B-0x40) */
    printf("4. Reading accelerometer (regs 0x3B-0x40):\n");
    for (int reg = 0x3B; reg <= 0x40; reg += 2) {
        unsigned char hi, lo;
        if (i2c_read_reg(fd, reg, &hi) == 0 &&
            i2c_read_reg(fd, reg + 1, &lo) == 0) {
            int16_t raw = (hi << 8) | lo;
            printf("   Reg 0x%02x-0x%02x = %d (raw)\n", reg, reg + 1, raw);
        }
    }

    close(fd);
    printf("\n=== Done ===\n");
    return 0;
}
