#include "scull.h"
#include <linux/uaccess.h>
#include <asm-generic/posix_types.h>

int scull_major = SCULL_INIT;
int scull_minor = SCULL_INIT;
struct scull_dev scull_dev;
dev_t scull_devnum;


struct file_operations scull_fops = {
	.owner  = THIS_MODULE,
	.read	= scull_read,
	.write	= scull_write,
	.open	= scull_open,
	.release	= scull_release,
};

static void scull_debug(struct scull_dev *dev)
{
	struct scull_qset *dptr = dev->data;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int i, j;

	while(NULL != dptr)
	{
		if(NULL != dptr->data)	
		{
			for(i = 0; i < qset; i++)
			{
				printk("qset[%d]: ", i);
				if(NULL != dptr->data[i])
				{
					for(j = 0; j < quantum; j++)
					{
						printk("%s", (char*)&dptr->data[i][j]);
					}
				}
				else
				{
					break;
				}
				printk("\n");
			}
		}
		dptr = dptr->next;
	}
}

static void scull_trim(struct scull_dev *dev)
{
	struct scull_qset *dptr, *next;
	int qset = dev->qset;
	int i;

	for (dptr = dev->qset; dptr; dptr = next)
	{
		if(dptr->data)
		{
			for(i = 0; i < qset; i++)
			{
				kfree(dptr->data[i]);
			}
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}

	dev->data = NULL;
	dev->quantum = SCULL_QUANTUM; 
	dev->qset = SCULL_QSET;
   	dev->size = SCULL_ZERO;
	dev->access_key = SCULL_ZERO;
}

static struct scull_qset * scull_follow(struct scull_dev *dev, int item)
{
	struct scull_qset *dptr = dev->data;

	printk("%s, %d, item = %d\n", __FUNCTION__, __LINE__, item);
	
	if(!dptr)
	{
		printk("%s, %d, scull_qset is Null, need malloc\n", __FUNCTION__, __LINE__);
		dptr = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if(NULL == dptr)
		{
			printk("%s, %d, kmalloc fail!\n", __FUNCTION__, __LINE__);
			goto err;
		}
		memset(dptr, 0, sizeof(struct scull_qset));
	}

	while(item--)
	{
		if(!dptr->next)
		{
			printk("%s, %d, scull_qset next is Null, need malloc\n", __FUNCTION__, __LINE__);
			dptr->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if (NULL == dptr->next)
			{
				printk("%s, %d, kmalloc fail!\n", __FUNCTION__, __LINE__);
				goto err;
			}
			memset(dptr->next, 0, sizeof(struct scull_qset));
		}
		dptr = dptr->next;
	}

err:
	return dptr;
}

ssize_t scull_read (struct file *filp, char *buf, size_t count,loff_t *f_pos)
{
	printk("%s %d, count=%d\n", __FUNCTION__, __LINE__, count);
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	ssize_t ret = 0;

	
	int quantum = dev->quantum;
	int qset = dev->qset;

	long itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;

	if(down_interruptible(&dev->sem))
	{
		printk("%s %d, Get sem fail!\n", __FUNCTION__, __LINE__);
		return -ERESTARTSYS;
	}

	if (*f_pos > dev->size)
	{
		printk("%s %d, Readsize out of buffer size\n", __FUNCTION__, __LINE__);
		ret = -1;
		goto err;
	}

	if((*f_pos + count) > dev->size)
	{
		count = dev->size - *f_pos; 
	}

	item = *f_pos / itemsize;
	rest = *f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if(!dptr || !dptr->data || !dptr->data[s_pos])
	{
		printk("%s, %d, Some pointer is NULL\n", __FUNCTION__, __LINE__);
		goto err;
	}

	if(count > quantum - q_pos)
	{
		count = quantum - q_pos;
	}
	
	ret = copy_to_user(buf, dptr->data[s_pos] + q_pos, count);
	if(ret)
	{
		printk("%s %d, copy_to_user fail!, ret=%d\n", __FUNCTION__, __LINE__, ret);
		goto err;
	}

	*f_pos += count;
	ret = count;
	printk("%s, %d, Read success, ret = %d\n", __FUNCTION__, __LINE__, ret);

err:
	up(&dev->sem);
	return ret;
}

ssize_t scull_write (struct file *filp, const char *buf, size_t count,loff_t *f_pos)
{
	printk("%s %d, count=%d\n", __FUNCTION__, __LINE__, count);
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	ssize_t ret = 0;

	
	int quantum = dev->quantum;
	int qset = dev->qset;

	long itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;


	if(down_interruptible(&dev->sem))
	{
		printk("%s %d, Get sem fail!\n", __FUNCTION__, __LINE__);
		return -ERESTARTSYS;
	}

	item = *f_pos / itemsize;
	rest = *f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	printk("%s, %d, itemsize[%d], quantum[%d], qset[%d], item[%d], *f_pos[%d], rest[%d], s_pos[%d], q_pos[%d]\n"\
			, __FUNCTION__, __LINE__, itemsize, quantum, qset, item, *f_pos, rest, s_pos, q_pos);
	dptr = scull_follow(dev, item);
	if(!dptr)
	{
		printk("%s, %d, Some pointer is NULL\n", __FUNCTION__, __LINE__);
		goto err;
	}

	if(!dptr->data)
	{
		printk("%s, %d, qset->data is NULL, need malloc\n", __FUNCTION__, __LINE__);
		dptr->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
		if(NULL == dptr->data)
		{
			printk("%s, %d, Some pointer is NULL\n", __FUNCTION__, __LINE__);
			goto err;
		}
	}

	if(!dptr->data[s_pos])
	{
		printk("%s, %d, qset->data[%d]] is NULL, need malloc\n", __FUNCTION__, __LINE__, s_pos);
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if(NULL == dptr->data[s_pos])
		{
			printk("%s, %d, Some pointer is NULL\n", __FUNCTION__, __LINE__);
			goto err;
		}

	}


	if(count > quantum - q_pos)
	{
		count = quantum - q_pos;
	}

	ret = copy_from_user(dptr->data[s_pos] + q_pos, buf,  count);
	if(ret)
	{
		printk("%s %d, copy_from_user fail!, ret=%d\n", __FUNCTION__, __LINE__, ret);
		goto err;
	}

	*f_pos += count;
	ret = count;
	if(dev->size < *f_pos)
	{
		dev->size = *f_pos;
	}

	printk("%s, %d, Write Success, ret=%d\n", __FUNCTION__, __LINE__, ret);

err:
	up(&dev->sem);
	return ret;
}

int scull_ioctl (struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
{
	printk("%s %d\n", __FUNCTION__, __LINE__);
	return 0;

}

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;
	printk("%s %d\n", __FUNCTION__, __LINE__);

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;

	if((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		if(down_interruptible(&dev->sem))
		{
			printk("%s %d, down_interruptible sem fail!\n", __FUNCTION__, __LINE__);
			return ERESTARTSYS;
		}
		scull_trim(dev);
		up(&dev->sem);
	}

	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	printk("%s %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static void scull_setup_dev(struct scull_dev *dev)
{
	int ret;
	// Step1: Init cdev "/dev/scull"
	if(scull_major)
	{
		ret = register_chrdev_region(MKDEV(scull_major, 0), 1, DEVNAME);
		scull_devnum = MKDEV(scull_major, 0);
	}
	else
	{
		ret = alloc_chrdev_region(&scull_devnum, 0, 1, DEVNAME);
		scull_major = MAJOR(scull_devnum);
		scull_minor = MINOR(scull_devnum);
	}

	if(ret)
	{
		printk("%s %d, Register fail! ret=%d\n", __FUNCTION__, __LINE__, ret);
		goto err;
	}

	printk("%s %d, Register success!", __FUNCTION__, __LINE__);
	printk("scull_major=%d, scull_minor=%d\n", scull_major, scull_minor);

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;

	ret = cdev_add(&dev->cdev, scull_devnum, 1);
	if(ret)
	{
		printk("%s %d, Register fail! ret=%d\n", __FUNCTION__, __LINE__, ret);
		goto err1;
	}

	// step2: Init ohter members.
	dev->data = NULL;
	dev->quantum = SCULL_QUANTUM; 
	dev->qset = SCULL_QSET;
   	dev->size = SCULL_ZERO;
	dev->access_key = SCULL_ZERO;
	sema_init(&dev->sem, 1);
	return ret;

err1:
	unregister_chrdev_region(scull_devnum, 1);
err:
	return ret;
}

static int scull_init(void)
{
	int ret;
	printk("%s %d \n", __FUNCTION__, __LINE__);
	memset(&scull_dev,0, sizeof(struct scull_dev));
	scull_setup_dev(&scull_dev);
/*
	ret = scull_pip_init();
	if(ret < 0)
	{
		printk("%s, scull_pip_init fail!\n", __FUNCTION__);
		goto fail;
	}
	ret = scull_device_init();
	if(ret < 0)
	{
		printk("%s, sscull_device_init fail!\n", __FUNCTION__);
		goto fail;
	}
*/
	ret = scull_bus_init();
	if(ret < 0)
	{
		printk("%s, sscull bus_init fail!\n", __FUNCTION__);
		goto fail;
	}

	printk("%s, register success, ret = %d\n", __FUNCTION__, ret);
	return 0;

fail:
	return ret;
}

static void scull_exit(void)
{
	printk("%s %d, scull_major: %d \n", __FUNCTION__, __LINE__, scull_major);
	unregister_chrdev_region(scull_devnum, 1);
	scull_bus_exit();
/*
	scull_device_exit();
	scull_pip_cleanup();
*/
}

module_init(scull_init);
module_exit(scull_exit);
MODULE_LICENSE("GPL");
