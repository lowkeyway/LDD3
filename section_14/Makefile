ifneq ($(KERNELRELEASE),)
	obj-m := ldd3_scullp.o
	ldd3_scullp-objs := scull.o scull_pip.o scull_device.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	@echo "Begain build..."

cleans:
	rm -rf *.o *.ko *.symvers *.mod.c *.order *.mod
	@echo "Clean linked files..."
endif
