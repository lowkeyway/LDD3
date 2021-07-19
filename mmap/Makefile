ifneq ($(KERNELRELEASE),)
	obj-m := mmap_kernel.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	@echo "Begain build..."

cleans:
	rm -rf *.o *.ko *.symvers *.mod.c *.order
	@echo "Clean linked files..."
endif
