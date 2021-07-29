#include "ovt_core.h"


dev_t dev_num;
struct cdev char_dev;
struct class *char_class;
struct device *char_device;


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
  printk("%s\n", __func__);
  return ret;
}
static ssize_t ovt_device_write(struct file *filp, const char __user *buf,
    size_t count, loff_t *f_pos)
{
  int ret = 0;
  printk("%s\n", __func__);
  return ret;
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


static int __init ovt_device_init(void)
{
  int ret = 0;
  printk("%s\n", __func__);

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

static void __exit ovt_device_exit(void)
{
  printk("%s\n", __func__);
  device_destroy(char_class, dev_num);
  class_destroy(char_class);
  cdev_del(&char_dev);
  unregister_chrdev_region(dev_num, 1);

  return;
}


module_init(ovt_device_init);
module_exit(ovt_device_exit);
MODULE_LICENSE("GPL");
