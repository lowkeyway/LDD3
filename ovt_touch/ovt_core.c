#include "ovt_core.h"

static bool plt_reg = 0;
static bool node_reg = 0;

extern const struct attribute_group ovt_attr_group;

static struct spi_board_info ovt_spi_board = {
  .modalias = "ovt_spi_device",
  .max_speed_hz = 10000000,
  .chip_select = 3,
  .mode = SPI_MODE_0,
};

static int ovt_spi_setup(struct spi_device *spi)
{
  printk("%s\n", __func__);
  printk("chip select, spi: %d, master: %d\n", spi->chip_select, spi->master->num_chipselect);
  return 0;
}

static int ovt_spi_transfer(struct spi_master *master, struct spi_message *msg)
{
  printk("%s\n", __func__);
  return 0;
}

static int ovt_core_probe(struct platform_device *pdev)
{
  int ret = 0;
  struct spi_master *master;
  struct spi_device *spi_dev;

  printk("%s\n", __func__);

  master = spi_alloc_master(&(pdev->dev), 0);
  if(!master) {
    printk("spi_alloc_master fail!\n");
    return -EINVAL;
  }

  master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST;
  master->num_chipselect = 4;
  master->transfer_one_message = ovt_spi_transfer;
  master->setup = ovt_spi_setup;

  if(spi_register_master(master)) {
    printk("spi_register_master fail!\n");
    return -EINVAL;
  }

  platform_set_drvdata(pdev, master);

  spi_dev = spi_new_device(master, &ovt_spi_board);
  if(NULL == spi_dev) {
    printk("spi_new_device fail!\n");
    return -EINVAL;
  }

  printk("Register SPI device finish!\n");

  ret = sysfs_create_group(&pdev->dev.kobj, &ovt_attr_group);
  if(ret){
    printk("sysfs_create_group fail! ret = %d\n", ret);
    return ret;
  }
  
  printk("Create node finish!\n");

  node_reg = 1;
  return ret;
}

static int ovt_core_remove(struct platform_device *pdev)
{
  printk("%s\n", __func__);
  if(node_reg)
   sysfs_remove_group(&pdev->dev.kobj, &ovt_attr_group);
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
  return 0;
}

module_init(ovt_core_init);
module_exit(ovt_core_exit);
MODULE_LICENSE("GPL");
