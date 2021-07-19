本例程主要是用来测试mmap功能，主要包括三个文件：
+ mmap_kernel.c: kernel driver；
+ Makefile: 用来编译mmap_kernel.c，生成ko文件；
+ mmap_test.c：用户态的测试程序，直接用gcc编译即可；



# 测试步骤：

## 1. 直接执行make

可以查看makefile文件，主要就是吧mmap_kernel.c编译成mmap_kernel.ko文件。
```
ifneq ($(KERNELRELEASE),)
	obj-m := mmap_kernel.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	@echo "Begain build..."

cleans:
	rm -rf *.o *.ko *.symvers *.mod.c *.order
	@echo "Clean linked files..."
endif

```

## 2. 运行mmap_kernel.ko

可能需要管理员权限：

```
sudo insmod ./mmap_kernel.ko
```

可以在在另外一个窗口中用"dmesg -w"命令查看log输出：
```
[  573.178399] memdev_init
[  573.178401] register chr dev success!
[  573.178403] Finish!!!
```


## 3. 查看并创建相应的节点

### 可以用下面命令查看设备号：
```
lowkeyway@lowkeyway:/dev$ll memdev_misc 
crw------- 1 root root 10, 54 7月  19 21:31 memdev_misc
```

## 4. 编译mmap_text.c

这个很简单，直接生成a.out就可以
```
gcc mmap_text.c
```


## 5. 运行test测试

```
sudo ./a.out
```

如果能看到下面的log，就证明mmap成功。

```
lowkeyway@lowkeyway:~/code/LDD/mmap$sudo ./a.out 
begain mmap
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 
   0    0    0    0    0    0    0    0    0    0 

   0    1    2    3    4    5    6    7    8    9 
  10   11   12   13   14   15   16   17   18   19 
  20   21   22   23   24   25   26   27   28   29 
  30   31   32   33   34   35   36   37   38   39 
  40   41   42   43   44   45   46   47   48   49 
  50   51   52   53   54   55   56   57   58   59 
  60   61   62   63   64   65   66   67   68   69 
  70   71   72   73   74   75   76   77   78   79 
  80   81   82   83   84   85   86   87   88   89 
  90   91   92   93   94   95   96   97   98   99
```


以上就是环境的验证，环境OK了，代码其实非常简单，可以认为三步走：

+ 1. 内核态：kernel中用kmalloc申请一块物理地址连续的内存。需要注意的是，mmap的最小单位是一个page，一般的内核的一个page都是4K大小。
+ 2. 内核态：文件的file_operations中增加mmap函数实例化。实例化的主要作用就是通过remap_pfn_range把kmalloc申请的内存（这里是物理地址连续的逻辑内存）映射物理地址上；
+ 3. 用户态：调用mmap接口就可以拿到内核态申请的物理地址对应的用户态的虚拟地址了；

通过上面三个步骤，就建立了内核态内存和用户态内存的联系，这样的访问就不用走copy_to_user这种memcpy接口了。


但是还遗留了几个问题需要思考：
1. 内核态如何通知用户态内存已经写好了？是不是还要通过ioctl来查询，这样岂不是没有节省系统调用带来的开支？
2. 因为mmap的最小单位是一个page（4K)这个开销是对很多driver来说是需要额外申请的，就意味着要在kernel里面做一次内存copy把数据先搬到这个mmap的地址中，这岂不是也浪费了一次copy动作？
