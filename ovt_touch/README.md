# Platform Device

我们知道，Linux中的Platform设备是抽象出来的一种总线设备，它可以承载很多具体的设备数据。
按照Linux 的设备、驱动、总线模型，我们知道，如果要完成一个platform设备驱动，需要三个步骤：
+ 注册device
+ 注册driver
+ 注册bus（这部分由kernel自动完成，我们可以不关心）

## Register Device 

其实注册一个platform device非常简单，一个函数就可以搞定，**platform_device_register**, 放在代码中可以参考：
```
static void ovt_plt_release(struct device *dev)
{
  printk("%s\n", __func__);
  return;
}

struct platform_device ovt_platform_device = {
  .name = OVT_PLATFORM_NAME,
  .id = -1,
  .dev = {
    .release = ovt_plt_release,
  },
};

platform_device_register(&ovt_platform_device);
```

反向的，注销一个platform device也很简单，**platform_device_unregister**就可以，代码可以参考：
```
platform_device_unregister(&ovt_platform_device);
```

## Register Driver

同Device的思路类似，如果我们想注册一个driver，也可以用一个函数搞定**platform_driver_register**，代码参考：
```
static struct platform_driver ovt_platform_driver = {
  .driver = {
    .name = OVT_PLATFORM_NAME,
    .owner = THIS_MODULE,
  },
  .probe = ovt_core_probe,
  .remove = ovt_core_remove,
};
platform_driver_register(&ovt_platform_driver);
```

注销platform driver，可以用**platform_driver_unregister**，代码参考：
```
platform_driver_unregister(&ovt_platform_driver);
```

## 节点

可以参考我们的makefile，编译的时候会生成ovt_core.ko和ovt_device.ko。
```
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
```
这样，make后我们只要在终端中分别加载两个ko文件就可以看到设备节点了：

```
$sudo insmod ovt_core.ko
$sudo insmod ovt_device.ko
```

```
lowkeyway@lowkeyway:/sys/devices/platform/OVT_TOUCH_PLT$ll
总用量 0
drwxr-xr-x  3 root root    0 7月  28 16:01 ./
drwxr-xr-x 12 root root    0 7月  28 16:01 ../
lrwxrwxrwx  1 root root    0 7月  28 16:01 driver -> ../../../bus/platform/drivers/OVT_TOUCH_PLT/
-rw-r--r--  1 root root 4096 7月  28 16:01 driver_override
-r--r--r--  1 root root 4096 7月  28 16:01 modalias
drwxr-xr-x  2 root root    0 7月  28 16:01 power/
lrwxrwxrwx  1 root root    0 7月  28 16:01 subsystem -> ../../../bus/platform/
-rw-r--r--  1 root root 4096 7月  28 16:01 uevent

```
