obj-m := gpio_driver.o

KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all clean overlay

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

overlay: gpio_overlay.dts
	dtc -@ -I dts -O dtb -o gpio_overlay.dtbo gpio_overlay.dts

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f gpio_overlay.dtbo
