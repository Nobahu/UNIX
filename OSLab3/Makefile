obj-m += tsulab.o

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean

install: all
	sudo insmod tsulab.ko
remove:
	sudo rmmod tsulab

test: install remove
	sudo dmesg | tail -5

.PHONY: all clean install remove test
