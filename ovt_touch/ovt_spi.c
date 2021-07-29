#include "ovt_core.h"

static bool plt_reg = 0;
static bool spi_reg = 0;


static void ovt_plt_release(struct device *dev)
{
  printk("%s\n", __func__);
  return;
}


static int ovt_spi_dev_probe(struct spi_device *spi)
{
  printk("%s\n", __func__);
  return 0;
}

static int ovt_spi_dev_remove(struct spi_device *spi)
{
  printk("%s\n", __func__);
  return 0;
}

struct platform_device ovt_platform_device = {
  .name = OVT_PLATFORM_NAME,
  .id = -1,
  .dev = {
    .release = ovt_plt_release,
  },
};




static struct spi_driver ovt_spi_driver = {
  .driver = {
    .name = "ovt_spi_device",
    .bus = &spi_bus_type,
    .owner = THIS_MODULE,
    .of_match_table = NULL,
  },
  .probe = ovt_spi_dev_probe,
  .remove = ovt_spi_dev_remove,
};


static int __init ovt_spi_init(void)
{
  int ret = 0;
  printk("%s\n", __func__);


  ret = platform_device_register(&ovt_platform_device);
  if(ret < 0)
    printk("Unable to add platform device\n");
  else
    plt_reg = 1;

  ret = spi_register_driver(&ovt_spi_driver);
  if(0 != ret)
    printk("Unable to add spi driver\n");
  else
    spi_reg = 1;

  return ret;
}

static void __exit ovt_spi_exit(void)
{
  printk("%s\n", __func__);
  if(plt_reg)
    platform_device_unregister(&ovt_platform_device);
  if(spi_reg)
    spi_unregister_driver(&ovt_spi_driver);
  return;
}


module_init(ovt_spi_init);
module_exit(ovt_spi_exit);
MODULE_LICENSE("GPL");
