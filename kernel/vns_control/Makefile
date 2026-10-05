# Kernel module Makefile for vns_control
obj-m += vns_control.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

load:
	sudo insmod vns_control.ko

unload:
	sudo rmmod vns_control

reload: unload load

.PHONY: all clean install load unload reload