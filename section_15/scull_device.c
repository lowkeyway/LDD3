#include "scull.h"
#include <linux/platform_device.h>
Scull_Device *g_scull_device = NULL;

static struct file_operations g_scull_misc_fops =
{
    .owner = THIS_MODULE,
    .open = NULL,
    .release = NULL,
    .unlocked_ioctl = NULL,
};

static struct miscdevice scull_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = SCULL_MISC_DEVICE,
    .fops = &g_scull_misc_fops,
};

static const struct file_operations scull_device_fops = {
	.owner = THIS_MODULE,
	.read  = scull_device_read,
	.write = scull_device_write,
};	

static ssize_t scull_device_sysfs_open_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
	ssize_t ret = 0;
	printk("%s %d, cout=%d\n", __func__, __LINE__, count);
	return ret;
}
static ssize_t scull_device_sysfs_open_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	printk("%s %d, cout=%d\n", __func__, __LINE__);
	return ret;

}
 struct device_attribute attrs = __ATTR(open, (S_IRUGO | S_IWUSR | S_IWGRP), scull_device_sysfs_open_show, scull_device_sysfs_open_store);

ssize_t scull_device_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t ret = 0;
	printk("%s %d, count=%d\n", __func__, __LINE__, count);

out:
	return ret;
}


ssize_t scull_device_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t ret = 0;
	printk("%s %d, buf=\"%s\"count=%d\n", __func__, __LINE__, buf, count);

out:
	return ret;

}

static int scull_device_mkclass(Scull_Device *dev)
{
	int ret = 0;
	struct class *cls = class_create(THIS_MODULE, SCULL_CLASS_PATH);
	if(IS_ERR(cls))
	{
		printk("%s %d, class creat fail!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto out;
	}
	dev->scull_class = cls;

	printk("%s %d, \"/sys/class/%s\"class creat success!\n", __func__, __LINE__, SCULL_CLASS_PATH);
out:
	return ret;
}

static int scull_device_rmclass(Scull_Device *dev)
{
	int ret = 0;
	if(dev->scull_class)
	{
		class_destroy(dev->scull_class);
		dev->scull_class = NULL;

		printk("%s %d, \"/sys/class/%s\"class remove success!\n", __func__, __LINE__, SCULL_CLASS_PATH);
	}

out:
	return ret;
}

static void scull_device_rmcdev(Scull_Cdev *dev)
{
	if(dev)
	{
		if(dev->chrdev_name)	
		{
			kfree(dev->chrdev_name);
			dev->chrdev_name = NULL;
		}

		cdev_del(&dev->chrdev);
		unregister_chrdev_region(dev->chrdev_no, 1);
		printk("%s %d, scull char device removed!\n", __func__, __LINE__);
	}
}

static int scull_device_mkcdev(Scull_Device *dev)
{
	int ret = 0;
	if(!dev)	
	{
		printk("%s %d, Scull_Device is NULL!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto out;
	}
	if(dev->scull_chrdev)
	{
		printk("%s %d, Scull_Cdev is exist!\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	
	Scull_Cdev *cdev = kzalloc(sizeof(Scull_Cdev), GFP_KERNEL);
	if(!cdev)
	{
		printk("%s %d, malloc Scull_Cdev fail\n", __func__, __LINE__);
		ret = -ENODEV;
		goto out;
	}

	ret = alloc_chrdev_region(&cdev->chrdev_no, 0, 1, SCULL_CHAR_DEVICE);
	if(ret < 0)
	{
		printk("%s %d, Fail to alloc chrdev!, ret=%d\n", __func__, __LINE__, ret);
		goto out1;
	}
	cdev->chrdev_major = MAJOR(cdev->chrdev_no);
	cdev->chrdev_minor = MINOR(cdev->chrdev_no);
	
	cdev_init(&cdev->chrdev, &scull_device_fops);

	ret = cdev_add(&cdev->chrdev, cdev->chrdev_no, 1);
	if(ret < 0)
	{
		printk("%s %d, cdev add fail! ret=%d\n", __func__, __LINE__, ret);
		goto out2;
	}

	cdev->chrdev_name = kzalloc(sizeof(SCULL_CHAR_DEVICE), GFP_KERNEL);
	if(!cdev->chrdev_name)
	{
		printk("%s %d, malloc chrdev_name fail!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto out2;
	}
	
	dev->scull_chrdev = cdev;
	printk("%s %d, register chrdevice success!\n", __func__, __LINE__);
	printk("scull_device_chrde: major=%d, minor=%d\n", cdev->chrdev_major, cdev->chrdev_minor);

	return ret;

out2:
	scull_device_rmcdev(cdev);

out1:
	if(cdev)
	{
		kfree(cdev);
		cdev=NULL;
	}
out:
	return ret;
}

int scull_device_init(void)
{
	int ret = 0;
	struct device *dev;
	if(NULL == g_scull_device)
	{
		g_scull_device = kzalloc(sizeof(Scull_Device), GFP_KERNEL);
		if(!g_scull_device)
		{
			printk("%s %d, malloc g_scull_device fail!\n", __func__, __LINE__);
			ret = ERESTARTSYS;
			goto out;
		}
	}

	ret = scull_device_mkclass(g_scull_device);
	if(ret < 0)
	{
		printk("%s %d, scull_device_mkclass fail! ret=%d", __func__, __LINE__, ret);
		goto out1;
	}

	ret = scull_device_mkcdev(g_scull_device);
	if(ret < 0)
	{
		printk("%s %d, scull_device_mkcdev fail! ret=%d", __func__, __LINE__, ret);
		goto out2;
	}

	dev = device_create(g_scull_device->scull_class, NULL, g_scull_device->scull_chrdev->chrdev_no, NULL, SCULL_CHAR_DEVICE "%d", g_scull_device->scull_chrdev->chrdev_major);
	if(IS_ERR(dev))
	{
		printk("%s %d, device create fail!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto out3;
	}

	g_scull_device->scull_sysfs = kobject_create_and_add(SCULL_SYS_DEVICE, NULL);
	if(!g_scull_device->scull_sysfs)
	{
		printk("%s %d, \"/sys/%s\" register fail!\n", __func__, __LINE__, SCULL_SYS_DEVICE);
		ret = -ENODEV;
		goto out4;
	}

	printk("%s %d, \"/sys/%s\" register success!\n", __func__, __LINE__, SCULL_SYS_DEVICE);

	ret = sysfs_create_file(g_scull_device->scull_sysfs, &attrs.attr);
	if(ret < 0)
	{
		printk("%s %d, create file fail!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto out5;
	}

	ret = misc_register(&scull_misc_dev);
	if(ret < 0)
	{
		printk("%s %d, misc register fail!\n", __func__, __LINE__);
		goto out6;
	}
	printk("%s %d, register \"/sys/class/misc/%s\" success!\n", __func__, __LINE__, SCULL_MISC_DEVICE);

	g_scull_device->scull_pltdev = platform_device_alloc(SCULL_PLATFORM_DEVICE, -1); 
	if(!g_scull_device->scull_pltdev)
	{
		printk("%s %d, alloc platform fail!\n", __func__, __LINE__);
		ret = -ENODEV;
		goto out7;
	}

	ret = platform_device_add(g_scull_device->scull_pltdev);
	if(ret < 0)
	{
		printk("%s %d, add platform fail!\n", __func__, __LINE__);
		goto out8;
	}

	printk("%s %d, register \"/sys/bus/platform/%s\" success!\n", __func__, __LINE__, SCULL_PLATFORM_DEVICE);

	printk("%s %d, success!\n", __func__, __LINE__);

	return ret;
	
out8:
	platform_device_put(g_scull_device->scull_pltdev);
out7:
	misc_deregister(&scull_misc_dev);
out6:

	sysfs_remove_file(g_scull_device->scull_sysfs, &attrs.attr);

out5:
	kobject_put(g_scull_device->scull_sysfs);

out4:
	device_destroy(g_scull_device->scull_class, g_scull_device->scull_chrdev->chrdev_no);

out3:
	scull_device_rmcdev(g_scull_device->scull_chrdev);
	if(g_scull_device->scull_chrdev)
	{
		kfree(g_scull_device->scull_chrdev);
		g_scull_device->scull_chrdev = NULL;
	}

out2:
	scull_device_rmclass(g_scull_device);
out1:
	if(g_scull_device)
	{
		kfree(g_scull_device);
		g_scull_device = NULL;
	}
out:	
	return ret;
}

int scull_device_exit(void)
{
	int ret = 0;
	misc_deregister(&scull_misc_dev);

	if(g_scull_device)
	{
		if(g_scull_device->scull_pltdev)
		{
			platform_device_unregister(g_scull_device->scull_pltdev);
			platform_device_put(g_scull_device->scull_pltdev);
		}

		if(g_scull_device->scull_class && g_scull_device->scull_chrdev)
			device_destroy(g_scull_device->scull_class, g_scull_device->scull_chrdev->chrdev_no);

		if(g_scull_device->scull_class)
		{
			ret = scull_device_rmclass(g_scull_device);
		}

		if(g_scull_device->scull_chrdev)
		{
			scull_device_rmcdev(g_scull_device->scull_chrdev);
			kfree(g_scull_device->scull_chrdev);
			g_scull_device->scull_chrdev = NULL;
		}

		if(g_scull_device->scull_sysfs)
		{
			sysfs_remove_file(g_scull_device->scull_sysfs, &attrs.attr);
			kobject_put(g_scull_device->scull_sysfs);
			printk("%s %d, \"/sys/%s\" remove success!\n", __func__, __LINE__, SCULL_SYS_DEVICE);
		}

		kfree(g_scull_device);
		g_scull_device = NULL;
	}

	printk("%s %d\n", __func__, __LINE__);
out:
	return ret;
}
