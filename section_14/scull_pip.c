#include <asm-generic/errno.h>
#include "scull.h"
#include <linux/uaccess.h>

/* DEFINE */
#ifndef min
#define min(a, b) ((a)<(b)?(a):(b))
#endif

/* */
Scull_Pip *scull_pip_dev = NULL;
int scull_p_buffer = SCULL_P_BUFFER;
int scull_p_major = 0;
int scull_p_minor = 0;
dev_t scull_p_devnum;

/* Struct */
struct file_operations scull_pip_fops = {
	.owner	= THIS_MODULE,
	.read	= scull_pip_read,
	.write	= scull_pip_write,
	.open	= scull_pip_open,
	.release = scull_pip_release,
};

/* FUNCTION */
ssize_t scull_pip_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	ssize_t ret = 0;
	Scull_Pip *dev = filp->private_data;
	printk("%s %d, count=%d, *f_pos=%d\n", __FUNCTION__, __LINE__, count, *f_pos);

	/*
	if(f_pos != &filp->f_pos)
	{
		printk("%s %d, f_pos not correct!\n", __FUNCTION__, __LINE__);
		ret = -ESPIPE;
		goto out;
	}
	*/

	if(down_interruptible(&dev->sem))
	{
		printk("%s %d, down_interruptible when interrupt.\n", __FUNCTION__, __LINE__);
		ret = -ERESTARTSYS;
		goto out;
	}

	if(!dev->buffer)
	{
		printk("%s, Buffer is Null!\n", __FUNCTION__);
		ret = -ERESTARTSYS;
		goto out;
	}

	while(dev->rp == dev->wp)
	{
		up(&dev->sem);
		if(filp->f_flags & O_NONBLOCK)
		{
			printk("%s %d, O_NONBLOCK return.\n", __FUNCTION__, __LINE__);
			ret = -EAGAIN;
			goto out;
		}

		printk("\"%s\" reading: going to sleep\n", current->comm);

		if(wait_event_interruptible(dev->inq, (dev->rp != dev->wp)))
		{
			printk("%s %d, wait_event_interruptible when interrupt.\n", __FUNCTION__, __LINE__);
			ret = ERESTARTSYS;
			goto out;
		}
		if(down_interruptible(&dev->sem))
		{
			printk("%s %d, down_interruptible when interrupt\n", __FUNCTION__, __LINE__);
			ret = ERESTARTSYS;
			goto out;

		}
	}

	if(dev->wp > dev->rp)
		count = min(count, dev->wp - dev->rp);
	else
		count = min(count, dev->end - dev->rp);

	if(copy_to_user(buf, dev->rp, count))
	{
		printk("%s %d, copy_to_user fail!\n", __FUNCTION__, __LINE__);
		up(&dev->sem);
		ret = -EFAULT;
		goto out;
	}
	dev->rp += count;
	if(dev->rp == dev->end)
		dev->rp = dev->buffer;

	up(&dev->sem);

	wake_up_interruptible(&dev->outq);
	
	printk("\"s\" did read %li bytes\n", current->comm, (long)count);
	return count;

out:
	msleep(200);
	return ret;
}

static inline int spacefree(Scull_Pip *dev)
{
	if(dev->rp == dev->wp)
		return dev->buffersize;
	return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) - 1;
}

ssize_t scull_pip_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	ssize_t ret = 0;
	Scull_Pip *dev = filp->private_data;

	printk("%s %d, count=%d, *f_pos=%d\n", __FUNCTION__, __LINE__, count, *f_pos);

	if(down_interruptible(&dev->sem))
	{
		printk("%s, down_interruptible as interrupt\n", __FUNCTION__);
		return -ERESTARTSYS;
	}

	while(spacefree(dev) == 0)
	{
		up(&dev->sem);
		if(filp->f_flags & O_NONBLOCK)
		{
			printk("%s, filp->f_flags: 0x%x\n", __FUNCTION__, filp->f_flags);
			ret = -EAGAIN;
			goto out;
		}
		printk("\"%s\" writing: going to sleep\n", current->comm);
		if(wait_event_interruptible(dev->outq, spacefree(dev) > 0))
		{
			printk("%s, wait_event_interruptible as interrupt\n", __FUNCTION__);
			ret = -ERESTARTSYS;
			goto out;
		}

		if(down_interruptible(&dev->sem))
		{
			printk("%s, %d, down_interruptible as interrupt\n", __FUNCTION__, __LINE__);
			ret = -ERESTARTSYS;
			goto out;
		}

	}

	count = min(count, spacefree(dev));
	if(dev->wp >= dev->rp)
		count = min(count, dev->end - dev->wp);
	else
		count = min(count, dev->rp - dev->wp -1);

	printk("Going to accept %li bytes to %p from %p\n", \
			(long)count, dev->wp, buf);

	if(copy_from_user(dev->wp, buf, count))
	{
		up(&dev->sem);
		printk("%s, %d, dcopy_from_user fail!\n", __FUNCTION__, __LINE__);
		ret =  -EFAULT;
		goto out;
	}
	dev->wp += count;
	if(dev->wp == dev->end)
		dev->wp = dev->buffer;

	up(&dev->sem);

	wake_up_interruptible(&dev->inq);
	return count;

out:
	return ret;
}
int scull_pip_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	printk("%s, %d, cmd=%d\n", __FUNCTION__, __LINE__, cmd);
out:
	return ret;
}
int scull_pip_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	Scull_Pip *dev;
	dev = container_of(inode->i_cdev, Scull_Pip, cdev);
	filp->private_data = dev;

	if(filp->f_mode & FMODE_READ)
		dev->nreaders++;
	if(filp->f_mode & FMODE_WRITE)
		dev->nwriters++;	

/*
	if((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		if(down_interruptible(&dev->sem))
		{
			printk("%s, down_interruptible fail!\n", __FUNCTION__);
			ret = ERESTARTSYS;
			goto out;
		}

		if(!dev->buffer)	
		{
			printk("%s, Malloc Buffer Here!\n", __FUNCTION__);
			dev->buffer = kmalloc(scull_p_buffer, GFP_KERNEL);
			if(!dev->buffer)
			{
				printk("%s, dev buffer malloc fail!\n", __FUNCTION__);
				ret = -ENOMEM;
				goto out;
			}
			dev->buffersize = scull_p_buffer;
			dev->end =dev->buffer + dev->buffersize;
			dev->rp = dev->buffer;
			dev->wp = dev->buffer;
		}

		up(&dev->sem);
	}
*/
out:
	return ret;
}

int scull_pip_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	Scull_Pip *dev = filp->private_data;
	down(&dev->sem);
	if(filp->f_mode & FMODE_READ)
		dev->nreaders--;
	if(filp->f_mode & FMODE_WRITE)
		dev->nwriters--;	
	up(&dev->sem);
out:
	return ret;
}

static int scull_pip_setup_dev(Scull_Pip *dev)
{
	int ret = 0;

	// Step 1: Init cdev "/dev/scullp"
	if(scull_p_major)
	{
		ret = register_chrdev_region(MKDEV(scull_p_major, 0), 1, SCULL_P_DEVNAME);
		scull_p_devnum = MKDEV(scull_p_major, 0);
	}
	else
	{
		ret = alloc_chrdev_region(&scull_p_devnum, 0, 1, SCULL_P_DEVNAME);
		scull_p_major = MAJOR(scull_p_devnum);
		scull_p_minor = MINOR(scull_p_devnum);
	}

	if(ret)
	{
		printk("%s %d, Register chrdev fail!, ret=%d", __FUNCTION__, __LINE__, ret);
		goto out;
	}


	cdev_init(&dev->cdev, &scull_pip_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_pip_fops;

	ret = cdev_add(&dev->cdev, scull_p_devnum, 1);
	if(ret)
	{
		printk("%s, add cdev fail!\n", __FUNCTION__);
		goto out;
	}

	printk("scull_p_major=%d, scull_p_minor=%d\n", scull_p_major, scull_p_minor);
	printk("%s, Register chrdev success!\n", __FUNCTION__, __LINE__);
out:
	return ret;	
}

int scull_pip_init()
{
	int ret = 0;
	if(!scull_pip_dev)
	{
		scull_pip_dev = kmalloc(sizeof(Scull_Pip), GFP_KERNEL);
		if(!scull_pip_dev)
		{
			printk("%s, scull_pip_dev malloc fail!\n", __FUNCTION__);
			ret = -ENOMEM;
			goto out;
		}
		memset(scull_pip_dev, 0, sizeof(Scull_Pip));
	}

	init_waitqueue_head(&scull_pip_dev->inq);
	init_waitqueue_head(&scull_pip_dev->outq);

/* Init this buffer when open device*/
	if(!scull_pip_dev->buffer)	
	{
		scull_pip_dev->buffer = kmalloc(scull_p_buffer, GFP_KERNEL);
		if(!scull_pip_dev->buffer)
		{
			printk("%s, scull_pip_dev buffer malloc fail!\n", __FUNCTION__);
			ret = -ENOMEM;
			goto out;
		}
		scull_pip_dev->buffersize = scull_p_buffer;
		scull_pip_dev->end = scull_pip_dev->buffer + scull_pip_dev->buffersize;
		scull_pip_dev->rp = scull_pip_dev->buffer;
		scull_pip_dev->wp = scull_pip_dev->buffer;
	}
/* */	
	sema_init(&scull_pip_dev->sem, 1);

	ret = scull_pip_setup_dev(scull_pip_dev);
	if(ret)
	{
		printk("%s, scull_pip_setup fail!\n", __FUNCTION__);
		ret = -1;
		goto out1;
	}

	return ret;

out1:
	if(scull_pip_dev)
	{
		if(scull_pip_dev->buffer)
		{
			kfree(scull_pip_dev->buffer);
		}
		kfree(scull_pip_dev);
		scull_pip_dev = NULL;
	}

out:
	return ret;
}

int scull_pip_cleanup(void)
{
	int ret = 0;
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	Scull_Pip *dev = scull_pip_dev;
	if(dev)
	{
		if(dev->buffer)
		{
			kfree(dev->buffer);
		}
		kfree(dev);
		dev = NULL;
	}

out:
	return ret;

}
