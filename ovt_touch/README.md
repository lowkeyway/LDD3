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

# Device Attrs

我们在做driver调试的过程中，经常会暴露一些节点，通过这些节点我们可以在用户态通过cat/echo命令同用户态进行通讯，那么这些节点在是如何做出来的呢？
其实也非常简单，只用一个函数就可以了**sysfs_create_group**(这个接口可以用来注册一组节点)，比如：

## 1. 先定义好需要的attr group，并且写好show/store函数
```
#include <linux/sysfs.h>
#include <linux/device.h>

static char buffer[10] = {0};

static ssize_t ovt_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  int ret = 0;
  printk("%s\n", __func__);
  ret = sprintf(buf, "%s", buffer);
  return ret; 
}

static ssize_t ovt_info_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
  int ret = 0;
  printk("%s, count = %d\n", __func__, count);

  if(count > 10) {
    printk("%s, count = %d\n", __func__, count);
    count = 10 -1;
  }
  
  memcpy(buffer, buf, count);

  return count; 
}

static DEVICE_ATTR(ovt_info, 0644, ovt_info_show, ovt_info_store);

static struct attribute *ovt_attributes[] = {
  &dev_attr_ovt_info.attr,
  NULL
};

const struct attribute_group ovt_attr_group = {
  .attrs = ovt_attributes,
};
```

## 2. 调用sysfs_create_group

在调用sysfs_create_group的时候，注意第一个传参，kobj，我们可以自己通过kobject_create_and_add函数创建一个节点，也可以利用已经写好的platform设备。

```
extern const struct attribute_group ovt_attr_group;
sysfs_create_group(&pdev->dev.kobj, &ovt_attr_group);
```


## 3. 查看节点

这样我们就可以在platform_device中看到 ovt_info 节点了。

```
lowkeyway@lowkeyway:/sys/devices/platform/OVT_TOUCH_PLT$cat ovt_info 
lowkeyway@lowkeyway:/sys/devices/platform/OVT_TOUCH_PLT$echo 1 > ovt_info 
lowkeyway@lowkeyway:/sys/devices/platform/OVT_TOUCH_PLT$cat ovt_info 
1
```