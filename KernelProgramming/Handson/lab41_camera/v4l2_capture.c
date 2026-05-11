/*
 * v4l2_capture.c — V4L2 Video Capture Userspace Program 🍓
 *
 * Opens a camera, captures a single YUYV frame, saves as raw file.
 * Works with USB webcams and RPi Camera Module (via libcamera/v4l2).
 *
 * Build: gcc -o v4l2_capture v4l2_capture.c
 * Usage: ./v4l2_capture [/dev/video0] [output.raw]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define WIDTH  640
#define HEIGHT 480
#define NUM_BUFFERS 4

struct buffer {
    void *start;
    size_t length;
};

static int xioctl(int fd, unsigned long request, void *arg)
{
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}

int main(int argc, char **argv)
{
    const char *dev = argc > 1 ? argv[1] : "/dev/video0";
    const char *out = argc > 2 ? argv[2] : "capture.raw";
    int fd, i;
    struct buffer buffers[NUM_BUFFERS];

    /* Open device */
    fd = open(dev, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Query capabilities */
    struct v4l2_capability cap;
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        perror("VIDIOC_QUERYCAP");
        close(fd);
        return 1;
    }

    printf("Driver:  %s\n", cap.driver);
    printf("Card:    %s\n", cap.card);
    printf("Bus:     %s\n", cap.bus_info);
    printf("Version: %u.%u.%u\n",
           (cap.version >> 16) & 0xFF,
           (cap.version >> 8) & 0xFF,
           cap.version & 0xFF);

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "Device does not support video capture\n");
        close(fd);
        return 1;
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "Device does not support streaming I/O\n");
        close(fd);
        return 1;
    }

    /* Enumerate formats */
    printf("\nSupported formats:\n");
    struct v4l2_fmtdesc fmtdesc = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
    while (xioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        printf("  %d: %s (%.4s)\n", fmtdesc.index,
               fmtdesc.description, (char *)&fmtdesc.pixelformat);
        fmtdesc.index++;
    }

    /* Set format */
    struct v4l2_format fmt = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .fmt.pix = {
            .width = WIDTH,
            .height = HEIGHT,
            .pixelformat = V4L2_PIX_FMT_YUYV,
            .field = V4L2_FIELD_NONE,
        },
    };

    if (xioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT");
        close(fd);
        return 1;
    }

    printf("\nCapture format: %ux%u, stride=%u, size=%u\n",
           fmt.fmt.pix.width, fmt.fmt.pix.height,
           fmt.fmt.pix.bytesperline, fmt.fmt.pix.sizeimage);

    /* Request buffers */
    struct v4l2_requestbuffers req = {
        .count = NUM_BUFFERS,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };

    if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("VIDIOC_REQBUFS");
        close(fd);
        return 1;
    }

    /* Map buffers */
    for (i = 0; i < (int)req.count; i++) {
        struct v4l2_buffer buf = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MMAP,
            .index = i,
        };

        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("VIDIOC_QUERYBUF");
            close(fd);
            return 1;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length,
                                PROT_READ | PROT_WRITE, MAP_SHARED,
                                fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return 1;
        }
    }

    /* Queue buffers */
    for (i = 0; i < (int)req.count; i++) {
        struct v4l2_buffer buf = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MMAP,
            .index = i,
        };
        if (xioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            close(fd);
            return 1;
        }
    }

    /* Start streaming */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("VIDIOC_STREAMON");
        close(fd);
        return 1;
    }

    /* Capture one frame */
    printf("\nCapturing frame...\n");
    struct v4l2_buffer buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };

    if (xioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        perror("VIDIOC_DQBUF");
        close(fd);
        return 1;
    }

    /* Save raw frame */
    FILE *fp = fopen(out, "wb");
    if (fp) {
        fwrite(buffers[buf.index].start, 1, buf.bytesused, fp);
        fclose(fp);
        printf("Saved %u bytes to %s\n", buf.bytesused, out);
        printf("Convert: ffmpeg -f rawvideo -pix_fmt yuyv422 "
               "-s %ux%u -i %s frame.png\n",
               fmt.fmt.pix.width, fmt.fmt.pix.height, out);
    }

    /* Stop streaming */
    xioctl(fd, VIDIOC_STREAMOFF, &type);

    /* Cleanup */
    for (i = 0; i < (int)req.count; i++)
        munmap(buffers[i].start, buffers[i].length);

    close(fd);
    printf("Done!\n");
    return 0;
}
