#include "ovt_core.h"

static bool plt_reg = 0;
static bool node_reg = 0;

extern const struct attribute_group ovt_attr_group;

static int ovt_core_probe(struct platform_device *pdev)
{
  int ret = 0;
  printk("%s\n", __func__);
  ret = sysfs_create_group(&pdev->dev.kobj, &ovt_attr_group);
  if(ret){
    printk("sysfs_create_group fail! ret = %d\n", ret);
    return ret;
  }
  node_reg = 1;
  return ret;
}

static int ovt_core_remove(struct platform_device *pdev)
{
  printk("%s\n", __func__);
  return 0;
}

static struct platform_driver ovt_platform_driver = {
  .driver = {
    .name = OVT_PLATFORM_NAME,
    .owner = THIS_MODULE,
  },
  .probe = ovt_core_probe,
  .remove = ovt_core_remove,
};

static int __init ovt_core_init(void)
{
  int ret = 0;
  printk("%s\n", __func__);
  ret = platform_driver_register(&ovt_platform_driver);
  if(ret)
    printk("Unable to register driver, ret = %d\n", ret);
  else
    plt_reg = 1;
  return ret;
}

static void __exit ovt_core_exit(void)
{
  printk("%s\n", __func__);
  if(plt_reg)
    platform_driver_unregister(&ovt_platform_driver);
  if(node_reg)
    sysfs_remove_group(&pdev->dev.kobj, &ovt_attr_group);
  return 0;
}

module_init(ovt_core_init);
module_exit(ovt_core_exit);
MODULE_LICENSE("GPL");
