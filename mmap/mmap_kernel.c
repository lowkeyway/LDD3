#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define MEMDEV_SIZE 4096//分配内存的大小
#define MEMDEV_NAME "memdev_misc"

/*mem设备描述结构体*/
struct mem_dev
{
  char *data; //分配到的内存的起始地址
  unsigned long size;  //内存的大小
};

char *kernel_message = "This is hello world message from kernel!\n";

struct mem_dev *mem_devp;
struct cdev cdev;

int mem_open(struct inode *inode, struct file *filp)
{
  filp->private_data = mem_devp;
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
  struct mem_dev *dev = filp->private_data;

  printk("%s\n", __func__);
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

static struct miscdevice g_mem_dev = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = MEMDEV_NAME,
  .fops = &mem_fops,
};

static int memdev_init(void)
{
  int result;
  int j;

  printk("%s\n", __func__);

  result = misc_register(&g_mem_dev);

  if(0 > result)
    return result;
  printk("register chr dev success!\n");
  mem_devp = kmalloc(sizeof(struct mem_dev), GFP_KERNEL);
  if(!mem_devp)
  {
    result = -ENOMEM;
    goto fail_malloc;
  }

  memset(mem_devp, 0, sizeof(struct mem_dev));
  mem_devp->size = MEMDEV_SIZE;
  mem_devp->data = kmalloc(MEMDEV_SIZE, GFP_KERNEL);
  memset(mem_devp->data, 0, MEMDEV_SIZE);
  //memcpy(mem_devp.data, kernel_message, strlen(kernel_message));
  //printk("len(kernel_message) = %d, data[%s]", strlen(kernel_message), mem_devp.data);
  for(j = 0; j < 100; j++)
  {
    mem_devp->data[j] = j;
  }

  printk("Finish!!!\n");

  return 0;

fail_malloc:
  printk("malloc fail!\n");
  misc_deregister(&g_mem_dev);
  return result;
}

static void memdev_exit(void)
{
  printk("%s\n", __func__);
  misc_deregister(&g_mem_dev);
  kfree(mem_devp->data);
  kfree(mem_devp);
}


MODULE_AUTHOR("Louis Li");
MODULE_LICENSE("GPL");
module_init(memdev_init);
module_exit(memdev_exit);
