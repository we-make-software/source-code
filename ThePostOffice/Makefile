KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

EXTRA_CFLAGS += -I../TheMailConditioner -I../ExpiryWorkBase
KBUILD_EXTRA_SYMBOLS := ../TheMailConditioner/Module.symvers ../ExpiryWorkBase/Module.symvers

COMMIT_MSG = Update on $(shell date '+%Y-%m-%d %H:%M:%S')

obj-m := ThePostOffice.o

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

commit:
	@if ! git diff-index --quiet HEAD; then \
		git add . && \
		git commit -m "$(COMMIT_MSG)" && \
		git push origin main; \
	else \
		echo "No changes in $(PWD) to commit."; \
	fi

pull:
	git pull origin main --rebase
