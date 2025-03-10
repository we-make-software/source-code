# Kernel version
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Module name
obj-m := ThePostOffice.o

# Build rules
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

log:
	dmesg -w

clear:
	dmesg -c


clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean


start:
	make all
	sudo insmod ThePostOffice.ko
	make log
	
stop:
	sudo rmmod ThePostOffice.ko
	make clear
