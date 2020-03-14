#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static char *whom = "world";
static int howmany = 3;

struct synaData{
	unsigned int data[10];
	int len;
};

static void synaTest(char *str, int len)
{
	printk("str[0-%d]: ", len);
	int i = 0;
	for(; i < len; i++)
	{
		printk("0x%pK ", str++);
	}
	printk("\n");
}

static int hello_init(void)
{
	int i;
	for(i = 0; i < howmany; i++)
	{
		printk("Hello %s!\n", whom);
	}
	struct synaData sData;
	synaTest((char *)sData.data, sizeof(sData.data));
	printk("sizeof synaData = %d\n", sizeof(sData));
	printk("Add of sData.data   is: 0x%pK", sData.data);
	printk("Add of sData.data+1 is: 0x%pK", sData.data+1);
	printk("Finish!\n");
	return 0;
}

static void hello_exit(void)
{
	printk("Bybe!\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("BSD/GPL");
module_param(howmany, int, 0644);
module_param(whom, charp, 0644);
