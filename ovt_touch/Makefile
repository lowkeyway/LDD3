ifneq ($(KERNELRELEASE),)
	obj-m := ovt_core.o ovt_device.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	@echo "Finish build..."

cleans:
	rm -rf *.o *.ko *.symvers *.mod.c *.order *.cmd
	@echo "Clean linked files..."
endif
