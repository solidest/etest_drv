**et_gpio驱动说明**

​		et_gpio设备驱动支持AXU3EG开发板上68路可配置DIO接口。程序主要接口说明如下：

**结构体**

et_gpio_data：gpio数据操作结构体

```c
struct et_gpio_data {

  unsigned int value;

  unsigned int mask;

};
```

et_gpio_dev:gpio自定义设备操作结构体

```c
struct et_gpio_dev {

  char name[16];

  dev_t devno;

  struct cdev cdev;

  struct device *device;

  unsigned int width;

  unsigned int * p_value;

  unsigned int * p_state;

  spinlock_t h_lock;

  \#ifdef ET_DEBUG

  unsigned int demo_value;

  unsigned int demo_state;

  \#endif

};
```

et_gpio_handle:自定义设备句柄

```C
struct et_gpio_handle {

  struct et_gpio_dev* dev;

  unsigned int used_pin;

  struct et_gpio_data poll_data;

  wait_queue_head_t h_poll;

  struct hrtimer timer;

  unsigned long timer_step;

};
```

**注册与注销函数及结构体**

- module_init(xxx_init);

功能：注册模块加载函数

- module_exit(xxx_init);

功能：注册模块卸载函数

- __init et_gpio_init(void)

功能：注册函数，作为module_init函数的参数使用。

- __exit et_gpio_exit(void)

功能：注销函数，作为module_exit函数的参数使用。

et_gpio_fops

功能：file_operations操作结构体。

```C
static struct file_operations et_gpio_fops = {

  .owner = THIS_MODULE,

  .open = et_gpio_open,

  .release = et_gpio_close,

  .write = et_gpio_write,

  .read = et_gpio_read,

  .unlocked_ioctl = et_gpio_ioctl,

  .poll = et_gpio_poll,

};
```

**IO操作函数**

```C
static int _poll_stop(struct et_gpio_handle* hdl)
```

功能：打开设备函数。

```C
static int et_gpio_close(struct inode * inode_p, struct file * file_p)
```

功能：关闭设备函数。

```C
static ssize_t et_gpio_write(struct file *file_p, const char __user *buf, size_t len, loff_t *loff_t_p)
```

功能：写GPIO寄存器函数。

```C
static ssize_t et_gpio_read(struct file *file_p, char __user *buf, size_t len, loff_t *loff_t_p)
```

功能：读GPIO寄存器函数。

**IOCTL控制函数**

```C
static long et_gpio_ioctl(struct file *file_p, unsigned int cmd, unsigned long arg)
```

功能：用cmd区分接口执行的操作。cmd有以下取值

​		IOC_SET_DIR：设置DIO方向。

​		IOC_GET_DIR：读取DIO方向。

​		IOC_POLL_START：开始轮询GPIO端口状态。

​		IOC_POLL_STOP：结束轮询GPIO端口状态。

​		IOC_POLL_GET：获得轮询状态。

**POLL操作函数**

```C
unsigned int et_gpio_poll(struct file *file_p, struct poll_table_struct *wait)
```

功能：轮询函数实现。

```C
static enum hrtimer_restart _do_poll(struct hrtimer *hrt)
```

功能：轮询函数实现，在et_gpio_open函数内调用。

```C
static int _poll_start(struct et_gpio_handle* hdl, unsigned long tick_us)
```

功能：轮询开始函数。

```C
static int _poll_stop(struct et_gpio_handle* hdl)
```

功能：轮询结束函数。