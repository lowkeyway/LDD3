ifneq ($(KERNELRELEASE),)
	obj-m := hello.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	@echo "Make default!"
cleans:
	rm -rf *.o *.ko *.symvers *.mod.c *.order
	@echo "Removing linked and compiled files......"
endif
