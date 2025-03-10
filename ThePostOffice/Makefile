# Kernel version
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Module name
obj-m := ThePostOffice.o

# Build rules
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean


start:
	sudo insmod ThePostOffice.ko

stop:
	sudo rmmod ThePostOffice.ko

log:
	dmesg -w
