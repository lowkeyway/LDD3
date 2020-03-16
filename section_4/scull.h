#include <asm-generic/ioctl.h>
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

/* DEFINE FOR SCULL_PIP */

#define SCULL_P_BUFFER 4000
#define SCULL_P_DEVNAME	"scullp"

typedef void* devfs_handle_t;

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

/* STRUCT FOR SCULL_PIP */
typedef struct scull_pip{
	wait_queue_head_t	inq, outq;
	char				*buffer, *end;
	int					buffersize;
	char				*rp, *wp;
	int					nreaders, nwriters;
	struct fasync_struct	*async_queue;
	struct semaphore	sem;
	devfs_handle_t		handle;
	struct cdev			cdev;
}Scull_Pip;

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

/* API FOR SCULL_PIPE */
int scull_pip_init(void);
ssize_t scull_pip_read (struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t scull_pip_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int     scull_pip_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
int scull_pip_open(struct inode *inode, struct file *filp); 
int scull_pip_release(struct inode *inode, struct file *filp);

#define __SCULL_H__
#endif
