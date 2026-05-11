obj-m := chardev_old.o chardev_new.o chardev_ioctl.o

KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all clean test_ioctl

all: modules test_ioctl

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

test_ioctl: test_ioctl.c
	gcc -Wall -o test_ioctl test_ioctl.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_ioctl
