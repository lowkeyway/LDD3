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


# SPI Device

在嵌入式开发的过程中，我们经常需要用到一些总线设计，比如IIC或者SPI之类的，那么他们是否也需要遵循device、bus、driver的架构呢？答案是肯定的！
我们尝试来看看SPI Device在linux中是如何实现的。

只是在DTS出现之后，其实我们很少能看到SPI Device的注册了，而且如何在ubuntu环境中探索SPI呢？

## Device

在SPI的概念中SPI master(controller)是要跟device绑定在一块的，所以如果我们抛开DTS，就需要自己建立一套SPI的master(controller)和device的结构。

## Master

从代码中就可以看到，spi的master和controller其实就是一个概念，它的定义可能会比较长，我还是都贴了出来，如下：
```
#define spi_master			spi_controller
{
	struct device	dev;

	struct list_head list;

	/* other than negative (== assign one dynamically), bus_num is fully
	 * board-specific.  usually that simplifies to being SOC-specific.
	 * example:  one SOC has three SPI controllers, numbered 0..2,
	 * and one board's schematics might show it using SPI-2.  software
	 * would normally use bus_num=2 for that controller.
	 */
	s16			bus_num;

	/* chipselects will be integral to many controllers; some others
	 * might use board-specific GPIOs.
	 */
	u16			num_chipselect;

	/* some SPI controllers pose alignment requirements on DMAable
	 * buffers; let protocol drivers know about these requirements.
	 */
	u16			dma_alignment;

	/* spi_device.mode flags understood by this controller driver */
	u32			mode_bits;

	/* bitmask of supported bits_per_word for transfers */
	u32			bits_per_word_mask;
#define SPI_BPW_MASK(bits) BIT((bits) - 1)
#define SPI_BPW_RANGE_MASK(min, max) GENMASK((max) - 1, (min) - 1)

	/* limits on transfer speed */
	u32			min_speed_hz;
	u32			max_speed_hz;

	/* other constraints relevant to this driver */
	u16			flags;
#define SPI_CONTROLLER_HALF_DUPLEX	BIT(0)	/* can't do full duplex */
#define SPI_CONTROLLER_NO_RX		BIT(1)	/* can't do buffer read */
#define SPI_CONTROLLER_NO_TX		BIT(2)	/* can't do buffer write */
#define SPI_CONTROLLER_MUST_RX		BIT(3)	/* requires rx */
#define SPI_CONTROLLER_MUST_TX		BIT(4)	/* requires tx */

#define SPI_MASTER_GPIO_SS		BIT(5)	/* GPIO CS must select slave */

	/* flag indicating this is an SPI slave controller */
	bool			slave;

	/*
	 * on some hardware transfer / message size may be constrained
	 * the limit may depend on device transfer settings
	 */
	size_t (*max_transfer_size)(struct spi_device *spi);
	size_t (*max_message_size)(struct spi_device *spi);

	/* I/O mutex */
	struct mutex		io_mutex;

	/* lock and mutex for SPI bus locking */
	spinlock_t		bus_lock_spinlock;
	struct mutex		bus_lock_mutex;

	/* flag indicating that the SPI bus is locked for exclusive use */
	bool			bus_lock_flag;

	/* Setup mode and clock, etc (spi driver may call many times).
	 *
	 * IMPORTANT:  this may be called when transfers to another
	 * device are active.  DO NOT UPDATE SHARED REGISTERS in ways
	 * which could break those transfers.
	 */
	int			(*setup)(struct spi_device *spi);

	/*
	 * set_cs_timing() method is for SPI controllers that supports
	 * configuring CS timing.
	 *
	 * This hook allows SPI client drivers to request SPI controllers
	 * to configure specific CS timing through spi_set_cs_timing() after
	 * spi_setup().
	 */
	int (*set_cs_timing)(struct spi_device *spi, struct spi_delay *setup,
			     struct spi_delay *hold, struct spi_delay *inactive);

	/* bidirectional bulk transfers
	 *
	 * + The transfer() method may not sleep; its main role is
	 *   just to add the message to the queue.
	 * + For now there's no remove-from-queue operation, or
	 *   any other request management
	 * + To a given spi_device, message queueing is pure fifo
	 *
	 * + The controller's main job is to process its message queue,
	 *   selecting a chip (for masters), then transferring data
	 * + If there are multiple spi_device children, the i/o queue
	 *   arbitration algorithm is unspecified (round robin, fifo,
	 *   priority, reservations, preemption, etc)
	 *
	 * + Chipselect stays active during the entire message
	 *   (unless modified by spi_transfer.cs_change != 0).
	 * + The message transfers use clock and SPI mode parameters
	 *   previously established by setup() for this device
	 */
	int			(*transfer)(struct spi_device *spi,
						struct spi_message *mesg);

	/* called on release() to free memory provided by spi_controller */
	void			(*cleanup)(struct spi_device *spi);

	/*
	 * Used to enable core support for DMA handling, if can_dma()
	 * exists and returns true then the transfer will be mapped
	 * prior to transfer_one() being called.  The driver should
	 * not modify or store xfer and dma_tx and dma_rx must be set
	 * while the device is prepared.
	 */
	bool			(*can_dma)(struct spi_controller *ctlr,
					   struct spi_device *spi,
					   struct spi_transfer *xfer);

	/*
	 * These hooks are for drivers that want to use the generic
	 * controller transfer queueing mechanism. If these are used, the
	 * transfer() function above must NOT be specified by the driver.
	 * Over time we expect SPI drivers to be phased over to this API.
	 */
	bool				queued;
	struct kthread_worker		kworker;
	struct task_struct		*kworker_task;
	struct kthread_work		pump_messages;
	spinlock_t			queue_lock;
	struct list_head		queue;
	struct spi_message		*cur_msg;
	bool				idling;
	bool				busy;
	bool				running;
	bool				rt;
	bool				auto_runtime_pm;
	bool                            cur_msg_prepared;
	bool				cur_msg_mapped;
	struct completion               xfer_completion;
	size_t				max_dma_len;

	int (*prepare_transfer_hardware)(struct spi_controller *ctlr);
	int (*transfer_one_message)(struct spi_controller *ctlr,
				    struct spi_message *mesg);
	int (*unprepare_transfer_hardware)(struct spi_controller *ctlr);
	int (*prepare_message)(struct spi_controller *ctlr,
			       struct spi_message *message);
	int (*unprepare_message)(struct spi_controller *ctlr,
				 struct spi_message *message);
	int (*slave_abort)(struct spi_controller *ctlr);

	/*
	 * These hooks are for drivers that use a generic implementation
	 * of transfer_one_message() provied by the core.
	 */
	void (*set_cs)(struct spi_device *spi, bool enable);
	int (*transfer_one)(struct spi_controller *ctlr, struct spi_device *spi,
			    struct spi_transfer *transfer);
	void (*handle_err)(struct spi_controller *ctlr,
			   struct spi_message *message);

	/* Optimized handlers for SPI memory-like operations. */
	const struct spi_controller_mem_ops *mem_ops;

	/* CS delays */
	struct spi_delay	cs_setup;
	struct spi_delay	cs_hold;
	struct spi_delay	cs_inactive;

	/* gpio chip select */
	int			*cs_gpios;
	struct gpio_desc	**cs_gpiods;
	bool			use_gpio_descriptors;

	/* statistics */
	struct spi_statistics	statistics;

	/* DMA channels for use with core dmaengine helpers */
	struct dma_chan		*dma_tx;
	struct dma_chan		*dma_rx;

	/* dummy data for full duplex devices */
	void			*dummy_rx;
	void			*dummy_tx;

	int (*fw_translate_cs)(struct spi_controller *ctlr, unsigned cs);

	/*
	 * Driver sets this field to indicate it is able to snapshot SPI
	 * transfers (needed e.g. for reading the time of POSIX clocks)
	 */
	bool			ptp_sts_supported;

	/* Interrupt enable state during PTP system timestamping */
	unsigned long		irq_flags;



```

+ 1. 我们可以通过**spi_alloc_master**接口来申请一个spi master。

需要注意的是spi_alloc_master中的size很有讲究，简单的说，他可以承载一些数据类型，跟要申请的master空间放在一块，直接贴在master后面。我不知道这种做法好还是不好，反正确实很新颖。Linux的确是个大宝库。

```
static inline struct spi_controller *spi_alloc_master(struct device *host,
						      unsigned int size)
{
	return __spi_alloc_controller(host, size, false);
}
```

```
struct spi_controller *__spi_alloc_controller(struct device *dev,
					      unsigned int size, bool slave)
{
	struct spi_controller	*ctlr;
	size_t ctlr_size = ALIGN(sizeof(*ctlr), dma_get_cache_alignment());

	if (!dev)
		return NULL;

	ctlr = kzalloc(size + ctlr_size, GFP_KERNEL);
	if (!ctlr)
		return NULL;

	device_initialize(&ctlr->dev);
	ctlr->bus_num = -1;
	ctlr->num_chipselect = 1;
	ctlr->slave = slave;
	if (IS_ENABLED(CONFIG_SPI_SLAVE) && slave)
		ctlr->dev.class = &spi_slave_class;
	else
		ctlr->dev.class = &spi_master_class;
	ctlr->dev.parent = dev;
	pm_suspend_ignore_children(&ctlr->dev, true);
	spi_controller_set_devdata(ctlr, (void *)ctlr + ctlr_size);

	return ctlr;
}

```

+ 2. 通过**spi_register_master**接口可以把申请的master挂在到设备总线上

具体什么意思呢，我们可以通过官方的一段描述来揣摩一下：

```
/**
 * spi_register_controller - register SPI master or slave controller
 * @ctlr: initialized master, originally from spi_alloc_master() or
 *	spi_alloc_slave()
 * Context: can sleep
 *
 * SPI controllers connect to their drivers using some non-SPI bus,
 * such as the platform bus.  The final stage of probe() in that code
 * includes calling spi_register_controller() to hook up to this SPI bus glue.
 *
 * SPI controllers use board specific (often SOC specific) bus numbers,
 * and board-specific addressing for SPI devices combines those numbers
 * with chip select numbers.  Since SPI does not directly support dynamic
 * device identification, boards need configuration tables telling which
 * chip is at which address.
 *
 * This must be called from context that can sleep.  It returns zero on
 * success, else a negative error code (dropping the controller's refcount).
 * After a successful return, the caller is responsible for calling
 * spi_unregister_controller().
 *
 * Return: zero on success, else a negative error code.
 */

```

+ 3. 通过**spi_new_device**接口把master和board info连接起来，并且返回一个spi device。

这里我们可以看一看board info是什么，就是这个东西，现在都写在了DTS里面。他定义了SPI CONTROLER当前链接的device的一些物理特性。比如名字、速率、芯片片选、工作模式等。
```
static struct spi_board_info ovt_spi_board = {
  .modalias = "ovt_spi_device",
  .max_speed_hz = 10000000,
  .chip_select = 3,
  .mode = SPI_MODE_0,
};
```

OK，通过上面几个步骤，其实我们就我已经完成了SPI DEVICE的注册，我们要记住的是“.modalias = "ovt_spi_device",” 这个名字将来在driver中也会使用。


## Driver

相比于device，driver部分就简单多了，一个函数搞定。 ---- **spi_register_driver**

```
static struct spi_driver ovt_spi_driver = {
  .driver = {
    .name = "ovt_spi_device",
    .bus = &spi_bus_type,
    .owner = THIS_MODULE,
    .of_match_table = NULL,
  },
  .probe = ovt_spi_dev_probe,
  .remove = ovt_spi_dev_remove,
};

spi_register_driver(&ovt_spi_driver);
```

## Node

一系列操作后，我们可以看到SPI对应的节点：

```
lowkeyway@lowkeyway:/sys/bus/spi/devices$ll
总用量 0
drwxr-xr-x 2 root root 0 7月  28 11:50 ./
drwxr-xr-x 4 root root 0 7月  28 11:50 ../
lrwxrwxrwx 1 root root 0 7月  29 12:45 spi0.3 -> ../../../devices/platform/OVT_TOUCH_PLT/spi_master/spi0/spi0.3
lrwxrwxrwx 1 root root 0 7月  29 12:45 spi1.3 -> ../../../devices/platform/OVT_TOUCH_PLT/spi_master/spi1/spi1.3/
```


# CDEV

我们在mmap中做过一个字符设备，使用杂项设备代替的，非常简单，一个msic_register就可以搞定了。但是有的时候，我们确实需要一个cdev设备，怎么办呢？
这个问题可难不倒我们，字符设备作为Linux驱动工程师的入门Hello World，随手就来一个。

## 步骤：

准备一下全局变量：
```
dev_t dev_num;
struct cdev char_dev;
struct class *char_class;
struct device *char_device;
```

### 1. 申请字符设备的设备号

```
alloc_chrdev_region(&dev_num, 0, 1, OVT_PLATFORM_NAME);
```

### 2. 字符设备初始化并关联字符设备号
```
static const struct file_operations ovt_device_fops = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = ovt_device_ioctl,
  .read = ovt_device_read,
  .write = ovt_device_write,
  .open = ovt_device_open,
  .release = ovt_device_release,
};

cdev_init(&char_dev, &ovt_device_fops);
ret = cdev_add(&char_dev, dev_num, 1);
```

### 3. 申请class，并创建设备
```
char_class = class_create(THIS_MODULE, OVT_PLATFORM_NAME);
char_device = device_create(char_class, NULL, dev_num, NULL, OVT_CHAR_NAME"%d", MINOR(dev_num));
```

## 节点

```
lowkeyway@lowkeyway:/dev$find . -name "OVT*"
./OVT_TOUCH_CHAR0
```

```
lowkeyway@lowkeyway:/sys$sudo find . -name "OVT*"
./class/OVT_TOUCH_PLT
./class/OVT_TOUCH_PLT/OVT_TOUCH_CHAR0
./devices/platform/OVT_TOUCH_PLT
./devices/virtual/OVT_TOUCH_PLT
./devices/virtual/OVT_TOUCH_PLT/OVT_TOUCH_CHAR0
./bus/platform/devices/OVT_TOUCH_PLT
./bus/platform/drivers/OVT_TOUCH_PLT
./bus/platform/drivers/OVT_TOUCH_PLT/OVT_TOUCH_PLT
```
