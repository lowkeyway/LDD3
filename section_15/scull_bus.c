#include "scull.h"


#define ldd_bus_version  "$revision : 2.0 $"
#define ldd_device_driver_name "sculld" 

typedef struct ldd_driver {
	char					*version;
	struct module			*module;
	struct device_driver	driver;
}Ldd_Driver;
#define to_ldd_driver(ptr) container_of(ptr, Ldd_Driver, driver)

typedef struct ldd_device {
	char				*name;
	struct ldd_driver	*driver;
	struct device		device;
}Ldd_Device;
#define to_ldd_device(ptr) container_of(ptr, Ldd_Device, device)


typedef struct sculld_dev {
	char				devname[256];
	Ldd_Device			ldev;
}Sculld_Dev;


struct bus_type ldd_bus_type = {
	.name		= "ldd",
	.match		= scull_bus_match,
	.uevent		= scull_bus_uevent,
};

struct device ldd_bus_device = {
	.init_name = "ldd0",
	.release = scull_bus_device_release,
};



static Ldd_Driver sculld_driver = {
	.version = ldd_bus_version,
	.module = THIS_MODULE,
	//.probe = scull_bus_driver_probe,
	.driver = {
		.name = ldd_device_driver_name,
	}
};

Sculld_Dev g_sculld_device;


void ldd_device_release(struct device *dev)
{
	printk("%s %d\n", __func__, __LINE__);
}


int register_ldd_device(Ldd_Device *ldddev)
{
	ldddev->device.bus = &ldd_bus_type;
	ldddev->device.parent = &ldd_bus_device;
	ldddev->device.release = ldd_device_release;
	//dev_set_name(&ldddev->device, ldddev->name);
	ldddev->device.init_name = ldddev->name;

	return device_register(&ldddev->device);
}
EXPORT_SYMBOL(register_ldd_device);

void unregister_ldd_device(Ldd_Device *ldddev)
{
	device_unregister(&ldddev->device);
}
EXPORT_SYMBOL(unregister_ldd_device);

int register_ldd_driver(Ldd_Driver *ldddrv)
{
	int ret = 0;

	ldddrv->driver.bus = &ldd_bus_type;
	ret = driver_register(&ldddrv->driver);
	if(ret)
	{
		printk("%s %d driver register fail!\n", __func__, __LINE__);
		goto out;
	}

	printk("%s %d driver register success!\n", __func__, __LINE__);
	return ret;

out:
	return ret;
}
EXPORT_SYMBOL(register_ldd_driver);

void unregister_ldd_driver(Ldd_Driver *ldddrv)
{
	driver_unregister(&ldddrv->driver);
}
EXPORT_SYMBOL(unregister_ldd_driver);

static int sculld_device_register(Sculld_Dev *dev, int index)
{
	int ret = 0;

	sprintf(dev->devname, "sculld%d", index);
	dev->ldev.name = dev->devname;
	dev->ldev.driver = &sculld_driver;
	dev->ldev.device.driver_data = dev;
	ret = register_ldd_device(&dev->ldev);
	if(ret)
	{
		printk("%s %d, register ldd device fail!\n", __func__, __LINE__);
		return ret;
	}
	printk("%s %d sculld device register success\n", __func__, __LINE__);

	return ret;
}

static void sculld_device_unregister(Sculld_Dev *dev)
{
	unregister_ldd_device(&dev->ldev);
}

static void scull_bus_device_release(struct device *dev)
{
	printk("%s %d, called!\n", __func__, __LINE__);
}


static int scull_bus_match(struct device *dev, struct device_driver *drv)
{
	int ret = !0;
	printk("%s %d, called!\n", __func__, __LINE__);

	if(!dev || !drv)
	{
		printk("%s %d, Device or Driver is Null\n", __func__, __LINE__);
	}
	if(dev->init_name != NULL)
	{
		printk("DeviceName: %s\n", dev->init_name);
		printk(" len DeviceName: %s\n", strlen(dev->init_name));
	}
	if(!drv->name)
	{
		printk("DriverName:%s\n", drv->name);
	}
	if(!dev->init_name && !drv->name)
	{
		printk("DeviceName: %s, DriverName:%s\n", dev->init_name, drv->name);
		return !strncmp(dev->init_name, drv->name, strlen(drv->name));
	}

out:
	return ret;

}

static int scull_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	int ret = 0;
	printk("%s %d, called!\n", __func__, __LINE__);
	env->envp[0] = env->buf;
	if(snprintf(env->buf, env->buflen, "LDDBUS_VERSION=%s, ldd_bus_version") >= env->buflen)
	{
		printk("%s %d, Put version fail\n", __func__, __LINE__);
		ret = -ENODEV;
		goto out;
	}

	env->envp[1] = NULL;
	
	return ret;

out:
	return ret;

}

int scull_bus_init()
{
	int ret = 0;

	ret = bus_register(&ldd_bus_type);
	if(ret)
	{
		printk("%s %d, ldd bus register fail!\n", __func__, __LINE__);
		goto out;
	}
	printk("%s %d, \"/sys/bus/ldd\" register success!\n", __func__, __LINE__);


	ret = device_register(&ldd_bus_device);
	if(ret)
	{
		printk("%s %d, ldd bus device register fail!\n", __func__, __LINE__);
		goto out1;
	}
	printk("%s %d, \"/sys/devices/ldd0\" register success!\n", __func__, __LINE__);
/*
	ret = driver_register(&ldd_bus_driver);
	if(ret)
	{
		printk("%s %d, ldd bus driver register fail!\n", __func__, __LINE__);
		goto out2;
	}
*/
	ret = sculld_device_register(&g_sculld_device, 0);
	if(ret)
	{
		printk("%s %d, sculld device register fail!\n", __func__, __LINE__);
		goto out2;
	}
	printk("%s %d, \"/sys/bus/ldd/devices/%s%d\" register success!\n", __func__, __LINE__, ldd_device_driver_name, 0);

	ret = register_ldd_driver(&sculld_driver);
	if(ret)
	{
		printk("%s %d, sculld driver register fail!\n", __func__, __LINE__);
		goto out3;
	}
	printk("%s %d, \"/sys/bus/ldd/drivers/%s\" register success!\n", __func__, __LINE__, ldd_device_driver_name);

	printk("%s %d, scull bus|device|driver init success!\n", __func__, __LINE__);

	return ret;

out3:
	sculld_device_unregister(&g_sculld_device);

out2:
	device_unregister(&ldd_bus_device);

out1:
	bus_unregister(&ldd_bus_type);

out:
	return ret;
}

int scull_bus_exit()
{
	int ret = 0;
/*
	driver_unregister(&ldd_bus_driver);
*/
	unregister_ldd_driver(&sculld_driver);
	sculld_device_unregister(&g_sculld_device);
	device_unregister(&ldd_bus_device);
	bus_unregister(&ldd_bus_type);
	printk("%s %d, scull bus exit success!\n", __func__, __LINE__);

out:
	return ret;
}
