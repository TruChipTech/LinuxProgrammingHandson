.PHONY: all clean

all: i2c_userspace i2c_scan

i2c_userspace: i2c_userspace.c
	gcc -Wall -o i2c_userspace i2c_userspace.c

i2c_scan: i2c_scan.c
	gcc -Wall -o i2c_scan i2c_scan.c

clean:
	rm -f i2c_userspace i2c_scan
