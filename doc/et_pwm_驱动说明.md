**et_pwm_drv驱动程序说明**

​	et_dwm设备驱动支持AXU3EG开发板上pwm接口。程序主要接口中说明如下：

**结构体**

alinx_char_dev：把驱动代码中会用到的数据打包进设备结构体。

```C
struct alinx_char_dev{

  dev_t       devid;    //设备号

  struct cdev    cdev;    //字符设备

  struct device_node *nd;     //设备树的设备节点

  unsigned int    *freq;    //频率的寄存器虚拟地址

  unsigned int    *duty;    //占空比的寄存器虚拟地址

};
```

alinx_char：声明设轩结构体。

```C
static struct alinx_char_dev alinx_char = {

  .cdev = {

     .owner = THIS_MODULE,

  },

};
```

**函数**

- ax_pwm_open

```C
static int ax_pwm_open(struct inode *inode_p, struct file *file_p)
```

功能：open函数实现，对应到Linux系统调用函数的open函数。

- ax_pwm_ioctl

```C
static long ax_pwm_ioctl(struct file *file_p, unsigned int cmd, unsigned long arg)
```

功能：iotcl函数实现，对应到Linux系统调用函数的iotcl函数。

- ax_pwm_release

```C
static int ax_pwm_release(struct inode *inode_p, struct file *file_p) 
```

功能：release函数实现，对应到Linux系统调用函数的close函数。

- file_operations结构体声明

```C
static struct file_operations ax_char_fops = { 

  .owner     = THIS_MODULE, 

  .open      = ax_pwm_open, 

  .unlocked_ioctl = ax_pwm_ioctl,   

  .release    = ax_pwm_release,  

};
```

- MISC设备结构体

```C
static struct miscdevice led_miscdev = {

  /* 自动分配次设备号 */

  .minor = MISC_DYNAMIC_MINOR,

  .name = DEVICE_NAME,

  /* file_operations结构体 */

  .fops = &ax_char_fops,

};
```

- ax_pwm_probe

```C
static int ax_pwm_probe(struct platform_device *dev)
```

功能：驱动和设备区配时会被调用。

- ax_pwm_remove

  ```C
  static int ax_pwm_remove(struct platform_device *dev)
  ```

功能：释放虚拟地址，注稍misc设备。

- pwm_of_match

```C
static const struct of_device_id pwm_of_match[] = {

  /* compatible字段和设备树中保持一致 */

  { .compatible = "alinx-pwm" },

  {/* Sentinel */}

};
```

功能：初始化of_match_table

- pwm_driver

```C
static struct platform_driver pwm_driver = {

  .driver = {

     /* name字段需要保留 */

     .name = "alinx-pwm",

     /* 用of_match_table代替name匹配 */

     .of_match_table = pwm_of_match,

  },

  .probe = ax_pwm_probe,

  .remove = ax_pwm_remove,

};
```

功能：声明并初始化platform驱动。

- pwm_drv_init

```C
static int __init pwm_drv_init(void)
```

功能：驱动入口函数。

- pwm_drv_exit

```C
static void __exit pwm_drv_exit(void)
```

功能：驱动出口函数。

**其它**

```C
/* 标记加载、卸载函数 */ 

module_init(pwm_drv_init);

module_exit(pwm_drv_exit);



/* 驱动描述信息 */ 

MODULE_AUTHOR("Alinx"); 

MODULE_ALIAS("pwm_led"); 

MODULE_DESCRIPTION("PWM LED driver"); 

MODULE_VERSION("v1.0"); 

MODULE_LICENSE("GPL"); 
```

