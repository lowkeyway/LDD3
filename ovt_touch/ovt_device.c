#include "ovt_core.h"

static bool plt_reg = 0;


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


static int __init ovt_device_init(void)
{
  int ret = 0;
  printk("%s\n", __func__);


  ret = platform_device_register(&ovt_platform_device);
  if(ret < 0)
    printk("Unable to add platform device\n");
  else
    plt_reg = 1;


  return ret;
}

static void __exit ovt_device_exit(void)
{
  printk("%s\n", __func__);
  if(plt_reg)
    platform_device_unregister(&ovt_platform_device);
 return;
}


module_init(ovt_device_init);
module_exit(ovt_device_exit);
MODULE_LICENSE("GPL");
