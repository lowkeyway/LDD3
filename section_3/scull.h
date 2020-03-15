#ifndef __SCULL_H__
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <uapi/asm-generic/fcntl.h>
#include <linux/semaphore.h>

/*
 * Define
 */
#define SCULL_INIT	0
#define DEVNAME		"scull"
#define SCULL_QUANTUM	512
#define SCULL_QSET		20
#define SCULL_ZERO		0

/*
 *  Struct
 */

struct scull_qset{
	void **data;
	struct scull_qset *next;
};

struct scull_dev{
	struct scull_qset *data;
	int quantum;
	int qset;
	unsigned long size;
	unsigned int access_key;
	struct semaphore sem;
	struct cdev cdev;
};

/*
 * Prototypes for shared functions
 */


ssize_t scull_read (struct file *filp, char *buf, size_t count,
                    loff_t *f_pos);
ssize_t scull_write (struct file *filp, const char *buf, size_t count,
                     loff_t *f_pos);
int     scull_ioctl (struct inode *inode, struct file *filp,
                     unsigned int cmd, unsigned long arg);
int scull_open(struct inode *inode, struct file *filp);
int scull_release(struct inode *inode, struct file *filp);
#define __SCULL_H__
#endif
