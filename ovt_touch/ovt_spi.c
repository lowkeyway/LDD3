#include "ovt_core.h"

static bool plt_reg = 0;
static bool spi_reg = 0;

extern struct ovt_tcm_hcd *tcm_hcd;
static struct ovt_tcm_hw_interface hw_if;
static struct ovt_tcm_bus_io bus_io;

static void ovt_plt_release(struct device *dev)
{
  printk("%s\n", __func__);
  return;
}

static int ovt_spi_read(struct ovt_tcm_hcd *tcm_hcd, unsigned char *data, unsigned int length)
{
  int i = 0;
  printk("%s, length = %d\n", __func__, length);
  for(i = 0; i < length; i++)
    data[i] = i;
  return 0;
}

static int ovt_spi_write(struct ovt_tcm_hcd *tcm_hcd, unsigned char *data, unsigned int length)
{
  int i = 0;
  printk("%s, length = %d\n", __func__, length);
  printk("write data: ");
  for(i = 0; i < length -1; i++) {
    if(0 == i%10) printk("\n");
    printk("%4d ", data[i]);
  }
  return length;
}



static int ovt_spi_dev_probe(struct spi_device *spi)
{
  printk("%s\n", __func__);
  hw_if.bdata = spi->dev.platform_data;
  bus_io.type = 0x1c;
  bus_io.read = ovt_spi_read;
  bus_io.write = ovt_spi_write;

  hw_if.bus_io = &bus_io;

  tcm_hcd->hw_if = &hw_if;
  tcm_hcd->pdev->dev.parent = &spi->dev;
  tcm_hcd->pdev->dev.platform_data = &hw_if;

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
