#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define MEMDEV_MAJOR 452/*预设的mem的主设备号*/
#define MEMDEV_NR_DEVS 2    /*设备数*/
#define MEMDEV_SIZE 4096//分配内存的大小

/*mem设备描述结构体*/
struct mem_dev
{
  char *data; //分配到的内存的起始地址
  unsigned long size;  //内存的大小
};

char *kernel_message = "This is hello world message from kernel!\n";

static int mem_major = MEMDEV_MAJOR;
module_param(mem_major, int, S_IRUGO);
struct mem_dev *mem_devp;
struct cdev cdev;

int mem_open(struct inode *inode, struct file *filp)
{
  struct mem_dev *dev;

  int num = MINOR(inode->i_rdev);
  if(num >= MEMDEV_NR_DEVS)
    return -ENODEV;

  dev = &mem_devp[num];
  filp->private_data = dev;
  return 0;
}

int mem_release(struct inode *inode, struct file *filp)
{
  printk("%s\n", __func__);
  return 0;
}

static ssize_t mem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
  printk("%s\n", __func__);
  return 0;
}

static ssize_t mem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
  printk("%s\n", __func__);
  return 0;
}

static loff_t mem_llseek(struct file *filp, loff_t offset, int whence)
{
  printk("%s\n", __func__);
  return 0;
}

static int memdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
  printk("%s\n", __func__);
  struct mem_dev *dev = filp->private_data;

  vma->vm_flags |= VM_IO;
  vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);
  if (remap_pfn_range(vma,vma->vm_start,virt_to_phys(dev->data)>>PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot))
          return  -EAGAIN;
  return 0;
}


static const struct file_operations mem_fops = 
{
  .owner = THIS_MODULE,
  .llseek = mem_llseek,
  .read = mem_read,
  .write = mem_write,
  .open = mem_open,
  .release = mem_release,
  .mmap = memdev_mmap,
};

static int memdev_init(void)
{
  int result;
  int i, j;
  dev_t devno = MKDEV(mem_major, 0);
  printk("%s\n", __func__);

  if(mem_major)
    result = register_chrdev_region(devno, 2, "memdev");
  else
  {
    result = alloc_chrdev_region(&devno, 0, 2, "memdev");
    mem_major = MAJOR(devno);
  }

  if(0 > result)
    return result;
  printk("register chr dev success!\n");
  cdev_init(&cdev, &mem_fops);
  cdev.owner = THIS_MODULE;
  cdev.ops = &mem_fops;

  cdev_add(&cdev, MKDEV(mem_major, 0), MEMDEV_NR_DEVS);

  mem_devp = kmalloc(MEMDEV_NR_DEVS * sizeof(struct mem_dev), GFP_KERNEL);
  if(!mem_devp)
  {
    result = -ENOMEM;
    goto fail_malloc;
  }

  memset(mem_devp, 0, sizeof(struct mem_dev));

  for(i = 0; i < MEMDEV_NR_DEVS; i++)
  {
    mem_devp[i].size = MEMDEV_SIZE;
    mem_devp[i].data = kmalloc(MEMDEV_SIZE, GFP_KERNEL);
    memset(mem_devp[i].data, 0, MEMDEV_SIZE);
    //memcpy(mem_devp[i].data, kernel_message, strlen(kernel_message));
    //printk("len(kernel_message) = %d, data[%d][%s]", strlen(kernel_message), i, mem_devp[i].data);
    for(j = 0; j < 100; j++)
    {
      mem_devp[i].data[j] = j;
    }
  }

  printk("Finish!!!\n");

  return 0;

fail_malloc:
  unregister_chrdev_region(devno, 1);
  return result;
}

static void memdev_exit(void)
{
  printk("%s\n", __func__);
  cdev_del(&cdev);
  kfree(mem_devp);
  unregister_chrdev_region(MKDEV(mem_major, 0), 2);
}


MODULE_AUTHOR("Louis Li");
MODULE_LICENSE("GPL");
module_init(memdev_init);
module_exit(memdev_exit);
