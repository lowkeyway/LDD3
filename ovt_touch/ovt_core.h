#ifndef __OVT_CORE_H_
#define __OVT_CORE_H_

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/cdev.h>

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

#endif
