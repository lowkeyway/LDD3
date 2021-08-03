#ifndef __OVT_CORE_H_
#define __OVT_CORE_H_

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define OVT_PLATFORM_NAME "OVT_TOUCH_PLT"
#define OVT_CHAR_NAME "OVT_TOUCH_CHAR"

struct ovt_tcm_board_data {
  unsigned int spi_mode;
};

struct ovt_tcm_bus_io {
  unsigned char type;
  int (*read)(struct ovt_tcm_hcd *tcm_hcd, unsigned char *data, unsigned int length);
  int (*write)(struct ovt_tcm_hcd *tcm_hcd, unsigned char *data, unsigned int length);
};

struct ovt_tcm_hw_interface {
  struct ovt_tcm_board_data *bdata;
  struct ovt_tcm_bus_io *bus_io;
};


struct ovt_tcm_hcd {
  struct platform_device *pdev;
  struct spi_device *spi_dev;
  struct ovt_tcm_hw_interface *hw_if;
};

struct device_hcd {
  struct cdev char_dev;
  struct ovt_tcm_hcd *tcm_hcd;
};

struct ovt_tcm_module_pool {
  bool queue_work;
  bool initialized;
  struct list_head list;
  struct work_struct work;
  struct workqueue_struct *workqueue;
  struct ovt_tcm_hcd *tcm_hcd;
};

enum module_type {
  TCM_DEVICE = 2,
  TCM_TESTING = 3,
};

struct ovt_tcm_module_cb {
  enum module_type type;
  int (*init)(struct ovt_tcm_hcd *tcm_hcd);
  int (*remove)(struct ovt_tcm_hcd *tcm_hcd);
};

struct ovt_tcm_module_handler {
  bool insert;
  bool detach;
  struct list_head list;
  struct ovt_tcm_module_cb *mod_cb;
};

#endif
