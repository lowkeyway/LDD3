#include "ovt_core.h"


dev_t dev_num;
struct cdev char_dev;
struct class *char_class;
struct device *char_device;
struct device_hcd *device_hcd;
extern struct ovt_tcm_hcd *tcm_hcd;

static unsigned char data[100] = {0};


static int ovt_device_open(struct inode *inp, struct file *filp)
{
  int ret = 0;
  printk("%s\n", __func__);
  return ret;
}

static int ovt_device_release(struct inode *inp, struct file *filp)
{
  int ret = 0;
  printk("%s\n", __func__);
  return ret;
}

static ssize_t ovt_device_read(struct file *filp, char __user *buf,
    size_t count, loff_t *f_pos)
{
  int ret = 0;
  size_t  length = count;
  printk("%s, count = %d\n", __func__, count);
  if(length > 100) {
    length = 99;
    data[99] = '\0';
  }

  tcm_hcd->hw_if->bus_io->read(tcm_hcd, data, length);
  copy_to_user(buf, data, length);
  return 100;
}

static ssize_t ovt_device_write(struct file *filp, const char __user *buf,
    size_t count, loff_t *f_pos)
{
  int ret = 0;
  printk("%s, count = %d\n", __func__);
  if(count > 100)
    count = 99;
  copy_from_user(data, buf, count);
  tcm_hcd->hw_if->bus_io->write(tcm_hcd, data, count);

  return count;
}
static long ovt_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  int ret = 0;
  printk("%s\n", __func__);
  return ret;
}


static const struct file_operations ovt_device_fops = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = ovt_device_ioctl,
  .read = ovt_device_read,
  .write = ovt_device_write,
  .open = ovt_device_open,
  .release = ovt_device_release,
};


static int device_init(struct ovt_tcm_hcd *tcm_hcd)
{
  int ret = 0;
  printk("%s\n", __func__);

  device_hcd = kzalloc(sizeof(*device_hcd), GFP_KERNEL);
  if(!device_hcd) {
    printk("alloc_device_hcd malloc fail!\n");
    return -ENOMEM;
  }


  ret = alloc_chrdev_region(&dev_num, 0, 1, OVT_PLATFORM_NAME);
  if(0 > ret) {
    printk("alloc_chardev_region fail!\n");
    goto err_alloc_chardev_region;
  }

  cdev_init(&char_dev, &ovt_device_fops);
  ret = cdev_add(&char_dev, dev_num, 1);
  if(0 > ret) {
    printk("alloc_chardev_region fail!\n");
    goto err_cdev_add;
  }

  char_class = class_create(THIS_MODULE, OVT_PLATFORM_NAME);
  if(NULL == char_class) {
    printk("class_create fail!\n");
    goto err_class_create;
  }

  char_device = device_create(char_class, NULL, dev_num, NULL, OVT_CHAR_NAME"%d", MINOR(dev_num));
  if(IS_ERR(char_device)) {
    ret = -ENODEV;
    printk("class_create fail!\n");
    goto err_device_create;
  }

  device_hcd->tcm_hcd = tcm_hcd;
  device_hcd->char_dev = char_dev;

  return 0;

err_device_create:
  class_destroy(char_class);

err_class_create:
  cdev_del(&char_dev);

err_cdev_add:
  unregister_chrdev_region(dev_num, 1);

err_alloc_chardev_region:

  return ret;
}

static int device_remove(struct ovt_tcm_hcd *tcm_hcd)
{
  printk("%s\n", __func__);
  device_destroy(char_class, dev_num);
  class_destroy(char_class);
  cdev_del(&char_dev);
  unregister_chrdev_region(dev_num, 1);

  return;
}


static struct ovt_tcm_module_cb device_module = {
  .type = TCM_DEVICE,
  .init = device_init,
  .remove = device_remove,
};

static int __init ovt_device_init(void)
{
  return ovt_tcm_add_module(&device_module, true);
}

static void __exit ovt_device_exit(void)
{
  ovt_tcm_add_module(&device_module, false);
}

module_init(ovt_device_init);
module_exit(ovt_device_exit);
MODULE_LICENSE("GPL");
