obj-m := i2c_driver_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all clean overlay

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

overlay: i2c_sensor_overlay.dts
	dtc -@ -I dts -O dtb -o i2c_sensor_overlay.dtbo i2c_sensor_overlay.dts

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.dtbo
