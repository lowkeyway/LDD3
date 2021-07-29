#include <linux/sysfs.h>
#include <linux/device.h>

static char buffer[10] = {0};

static ssize_t ovt_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  int ret = 0;
  printk("%s\n", __func__);
  ret = sprintf(buf, "%s", buffer);
  return ret; 
}

static ssize_t ovt_info_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
  int ret = 0;
  printk("%s, count = %d\n", __func__, count);

  if(count > 10) {
    printk("%s, count = %d\n", __func__, count);
    count = 10 -1;
  }
  
  memcpy(buffer, buf, count);

  return count; 
}

static DEVICE_ATTR(ovt_info, 0644, ovt_info_show, ovt_info_store);

static struct attribute *ovt_attributes[] = {
  &dev_attr_ovt_info.attr,
  NULL
};

const struct attribute_group ovt_attr_group = {
  .attrs = ovt_attributes,
};


