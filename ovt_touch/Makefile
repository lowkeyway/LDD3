ifneq ($(KERNELRELEASE),)
	obj-m := ovt.o ovt_spi.o ovt_device.o
	ovt-objs:= ovt_core.o ovt_node.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	@echo "Finish build..."

cleans:
	rm -rf *.o *.ko *.symvers *.mod.c *.order *.cmd
	@echo "Clean linked files..."
install:
	sudo insmod ovt.ko
	sudo insmod ovt_spi.ko
	sudo insmod ovt_device.ko

rmmod:
	sudo rmmod ovt
	sudo rmmod ovt_spi
	sudo rmmod ovt_device
endif
