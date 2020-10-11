/** ===================================================== **
 *Author : solidest
 *Website: http://www.Kiyun.com
 *Created: 2020-9-16 
 *Version: 1.0
 ** ===================================================== **/


#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/types.h>  
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/platform_device.h>

//TODO 定义设备资源数据，为驱动提供参数
static struct resource et_gpio_resources[] = 
{
    {
        .start = 0,
        .end = 4,
        .flags = IORESOURCE_MEM,
    }
};

static struct platform_device et_gpio_device = 
{
    .name = "et-gpio",
    .id = -1,
    .num_resources = ARRAY_SIZE(et_gpio_resources),
    .resource = et_gpio_resources,
};

static int __init et_gpio_device_init(void)
{
    return platform_device_register(&et_gpio_device);
}

static void __exit et_gpio_device_exit(void)
{
    platform_device_unregister(&et_gpio_device);
}

module_init(et_gpio_device_init);
module_exit(et_gpio_device_exit);
  
MODULE_AUTHOR("Kiyun");  
MODULE_ALIAS("et_gpio");  
MODULE_DESCRIPTION("et_gpio device");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  