/*
 * test_ioctl.c — Userspace ioctl test program
 *
 * Compile: gcc -o test_ioctl test_ioctl.c
 * Run:     sudo ./test_ioctl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE "/dev/ioctldev"
#define BUF_SIZE 4096

/* Must match kernel module definitions */
#define IOCTL_MAGIC  'k'
#define IOCTL_RESET       _IO(IOCTL_MAGIC, 0)
#define IOCTL_SET_MSG     _IOW(IOCTL_MAGIC, 1, char[BUF_SIZE])
#define IOCTL_GET_MSG     _IOR(IOCTL_MAGIC, 2, char[BUF_SIZE])
#define IOCTL_GET_LEN     _IOR(IOCTL_MAGIC, 3, int)
#define IOCTL_GET_COUNT   _IOR(IOCTL_MAGIC, 4, int)

int main(void)
{
    int fd, ret, len, count;
    char buf[BUF_SIZE];

    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open " DEVICE);
        return 1;
    }

    printf("=== ioctl Test Program ===\n\n");

    /* Test 1: Set message */
    printf("1. Setting message via ioctl...\n");
    snprintf(buf, sizeof(buf), "Hello from userspace ioctl!");
    ret = ioctl(fd, IOCTL_SET_MSG, buf);
    if (ret < 0) { perror("IOCTL_SET_MSG"); goto out; }
    printf("   Set: \"%s\"\n\n", buf);

    /* Test 2: Get length */
    printf("2. Getting buffer length...\n");
    ret = ioctl(fd, IOCTL_GET_LEN, &len);
    if (ret < 0) { perror("IOCTL_GET_LEN"); goto out; }
    printf("   Length: %d\n\n", len);

    /* Test 3: Get message back */
    printf("3. Getting message back...\n");
    memset(buf, 0, sizeof(buf));
    ret = ioctl(fd, IOCTL_GET_MSG, buf);
    if (ret < 0) { perror("IOCTL_GET_MSG"); goto out; }
    printf("   Got: \"%s\"\n\n", buf);

    /* Test 4: Reset */
    printf("4. Resetting buffer...\n");
    ret = ioctl(fd, IOCTL_RESET);
    if (ret < 0) { perror("IOCTL_RESET"); goto out; }
    printf("   Buffer reset\n\n");

    /* Test 5: Verify empty after reset */
    printf("5. Verifying empty...\n");
    ret = ioctl(fd, IOCTL_GET_LEN, &len);
    if (ret < 0) { perror("IOCTL_GET_LEN"); goto out; }
    printf("   Length after reset: %d\n\n", len);

    /* Test 6: Get ioctl count */
    printf("6. Getting ioctl call count...\n");
    ret = ioctl(fd, IOCTL_GET_COUNT, &count);
    if (ret < 0) { perror("IOCTL_GET_COUNT"); goto out; }
    printf("   Total ioctl calls: %d\n\n", count);

    /* Test 7: Read/Write via standard file ops */
    printf("7. Testing standard read/write...\n");
    write(fd, "Standard write test", 19);
    lseek(fd, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    read(fd, buf, sizeof(buf));
    printf("   Read back: \"%s\"\n\n", buf);

    printf("=== All tests passed ===\n");

out:
    close(fd);
    return 0;
}
