#include "ovt_core.h"

static bool plt_reg = 0;
static bool node_reg = 0;

extern const struct attribute_group ovt_attr_group;
struct ovt_tcm_hcd *tcm_hcd;
EXPORT_SYMBOL(tcm_hcd);
static struct ovt_tcm_module_pool mod_pool;

static struct spi_board_info ovt_spi_board = {
  .modalias = "ovt_spi_device",
  .max_speed_hz = 10000000,
  .chip_select = 3,
  .mode = SPI_MODE_0,
};

int ovt_tcm_add_module(struct ovt_tcm_module_cb *mod_cb, bool insert)
{
  printk("%s\n", __func__);
  struct ovt_tcm_module_handler *mod_handler;

  
  if(!mod_pool.initialized) {
    INIT_LIST_HEAD(&mod_pool.list);
    mod_pool.initialized = true;
  }

  if(insert) {
    mod_handler = kzalloc(sizeof(*mod_handler), GFP_KERNEL);
    if(!mod_handler) {
      printk("zalloc mod_handler fail!\n");
      return -ENOMEM;
    }

    mod_handler->mod_cb = mod_cb;
    mod_handler->insert = true;
    mod_handler->detach = false;
    list_add_tail(&mod_handler->list, &mod_pool.list);
 } else {
    if(!list_empty(&mod_pool.list)) {
      list_for_each_entry(mod_handler, &mod_pool.list, list) {
        if(mod_handler->mod_cb->type == mod_cb->type) {
          mod_handler->insert = false;
          mod_handler->detach = true;
        }
      }
      
    }
 }

  if(mod_pool.queue_work) {
    queue_work(mod_pool.workqueue, &mod_pool.work);
  }

  return 0;
}

EXPORT_SYMBOL(ovt_tcm_add_module);

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


static int ovt_spi_bus_device(struct platform_device *pdev, struct spi_device **spi_dev)
{
  int ret = 0;
  struct spi_master *master;

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

  //platform_set_drvdata(pdev, master);

  *spi_dev = spi_new_device(master, &ovt_spi_board);
  if(NULL == *spi_dev) {
    printk("spi_new_device fail!\n");
    return -EINVAL;
  }

  printk("Register SPI device finish!\n");

  return ret;
}

static int ovt_core_probe(struct platform_device *pdev)
{
  int ret = 0;
  struct spi_device *spi_dev;

  tcm_hcd = kzalloc(sizeof(*tcm_hcd), GFP_KERNEL);
  if(NULL == tcm_hcd) {
    printk("tcm_hcd malloc fail!\n");
    return -ENOMEM;
  }

  printk("%s\n", __func__);

  tcm_hcd->pdev = pdev;

  ret = ovt_spi_bus_device(pdev, &spi_dev);
  if(ret) {
    printk("ovt_spi_bus_device fail! ret = %d\n", ret);
    return ret;
  }

  tcm_hcd->spi_dev = spi_dev;

  platform_set_drvdata(pdev, tcm_hcd);

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
  struct spi_master *master = tcm_hcd->spi_dev->master;
  spi_unregister_device(tcm_hcd->spi_dev);
  spi_unregister_master(master);

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

static void ovt_tcm_module_work(struct work_struct *work)
{
  printk("%s\n", __func__);
  struct ovt_tcm_module_handler *mod_handler;
  struct ovt_tcm_module_handler *temp_handler;
  struct ovt_tcm_hcd *tcm_hcd = mod_pool.tcm_hcd;

  if(!list_empty(&mod_pool.list)) {
    list_for_each_entry_safe(mod_handler, temp_handler, &mod_pool.list, list) {
      if(mod_handler->insert) {
        if(mod_handler->mod_cb->init)
          mod_handler->mod_cb->init(tcm_hcd);
        mod_handler->insert = false;
      }
      if(mod_handler->detach) {
        if(mod_handler->mod_cb->remove)
          mod_handler->mod_cb->remove(tcm_hcd);
        mod_handler->detach = false;
        list_del(&mod_handler->list);
        kfree(mod_handler);
      }

    }
  }

  return;
}

static int __init ovt_core_init(void)
{
  int ret = 0;
  printk("%s\n", __func__);
  ret = platform_driver_register(&ovt_platform_driver);
  if(ret) {
    printk("Unable to register driver, ret = %d\n", ret);
    goto err;
  }
  else {
    plt_reg = 1;
  }

  mod_pool.workqueue = create_singlethread_workqueue("ovt_tcm_module");
  INIT_WORK(&mod_pool.work, ovt_tcm_module_work);
  mod_pool.queue_work = true;

err:
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
