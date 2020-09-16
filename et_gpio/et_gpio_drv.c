/** ===================================================== **
 *Author : solidest
 *Website: http://www.Kiyun.com
 *Created: 2020-9-16 
 *Version: 1.0
 ** ===================================================== **/

#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/ide.h>  
#include <linux/types.h>  
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#define ET_GPIO_NAME        "et_gpio"
#define ET_GPIO_MAJOR       300
#define GPIO_DEV_COUNT      2
#define GPIO_DEV_BASE0      0x80050000
#define GPIO_DEV_BASE1      0x80060000
#define GPIO_DEV_SIZE       0x1000
#define GPIO_DEV_CHCOUNT0   2
#define GPIO_DEV_CHCOUNT1   2
#define GPIO_DEV_WIDTH0_1   17
#define GPIO_DEV_WIDTH0_2   17
#define GPIO_DEV_WIDTH1_1   17
#define GPIO_DEV_WIDTH1_2   17

#define IOC_MAGIC_DIO 'd'
#define IOC_SET_DIR _IOW(IOC_MAGIC_DIO, 1, long)
#define IOC_GET_DIR _IOWR(IOC_MAGIC_DIO, 2, long)

struct et_gpio_dir{
	unsigned int ch_id;
    unsigned int io_dir;
    unsigned int mask;
};

struct et_gpio_value{
	unsigned int ch_id;
    unsigned int io_value;
    unsigned int mask;
};

struct et_gpio_dev {
    char name[16];
    dev_t devno;
    struct cdev cdev;
    struct device *device;
    unsigned long addr_base;
    unsigned int ch_count;
    unsigned int ch_width[2];
};



/////////////////////////////

static struct class * et_gpio_class;  
static struct et_gpio_dev et_gpio_devs[GPIO_DEV_COUNT];

static int et_gpio_open(struct inode * inode_p, struct file * file_p)
{
    // printk("%s : %d\n", __func__, __LINE__);
    struct et_gpio_dev * dev = container_of(inode_p->i_cdev, struct et_gpio_dev, cdev);
    if (IS_ERR(dev)) {
        return PTR_ERR(dev);
    }
    file_p->private_data = dev;
    //TODO 全部设置为输出方向
    return 0;
}

static int et_gpio_close(struct inode * inode_p, struct file * file_p)
{
    printk("%s : %d\n", __func__, __LINE__);

    return 0;
}

static ssize_t et_gpio_write(struct file *file_p, const char __user *buf, size_t len, loff_t *loff_t_p)
{
    struct et_gpio_dev * dev = (struct et_gpio_dev *)file_p->private_data;
    struct et_gpio_value v;

    if(len != sizeof(struct et_gpio_value)) {
        return -1;
    }

    if (IS_ERR(dev))
        return PTR_ERR(dev);
    copy_from_user(&v, buf, len);

    //TODO 写入输出值
    printk("写出 %d:%d:%d", v.ch_id, v.io_value, v.mask);
    return 0;
}

static ssize_t et_gpio_read(struct file *file_p, char __user *buf, size_t len, loff_t *loff_t_p)
{
    struct et_gpio_dev * dev = file_p->private_data;
    struct et_gpio_value v;

    if(len != sizeof(struct et_gpio_value)) {
        return -1;
    }

    if (IS_ERR(dev))
        return PTR_ERR(dev);
    copy_from_user(&v, buf, len);

    //TODO 读取输入值
    printk("读取 %d", v.ch_id);
    v.io_value = 100;
    v.mask = 200;
    copy_to_user(buf, &v, len);
    return 0;
}


static long et_gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    struct et_gpio_dev * dev = file->private_data;
    if (IS_ERR(dev))
        return PTR_ERR(dev);

    switch (cmd) {
        case IOC_SET_DIR: {
            struct et_gpio_dir dir;
            copy_from_user((unsigned char *)&dir, (unsigned char *)arg, sizeof(struct et_gpio_dir));
            //TODO 设置输入输出方向
            printk("设置输入输出方向: %d:%d:%d", dir.ch_id, dir.io_dir, dir.mask);
            break;
        }

        case IOC_GET_DIR: {
            struct et_gpio_dir demo = {
                .ch_id = 100,
                .io_dir = 200,
                .mask = 300
            };
            //TODO 获取输入输出方向
            copy_to_user((unsigned char *)arg, (unsigned char *)&demo, sizeof(struct et_gpio_dir));
            break;
        }

        default:
            retval = -EINVAL;
            break;
    }

    return retval;
}

///////////////////////////////////////

static struct file_operations et_gpio_fops = {
    .owner = THIS_MODULE,
    .open  = et_gpio_open,
    .release = et_gpio_close,
    .write = et_gpio_write,
    .read = et_gpio_read,
    .unlocked_ioctl = et_gpio_ioctl,
};

static int __init et_gpio_init(void)  
{  
    int i;
    printk("%s : %d\n", __func__, __LINE__);

    for (i = 0; i < GPIO_DEV_COUNT; i++) {
        et_gpio_devs[i].devno = MKDEV(ET_GPIO_MAJOR, i);
        sprintf(et_gpio_devs[i].name, "%s%d", ET_GPIO_NAME, i);
        register_chrdev_region(et_gpio_devs[i].devno, 1, et_gpio_devs[i].name);
        et_gpio_devs[i].cdev.owner = THIS_MODULE;
        cdev_init(&et_gpio_devs[i].cdev, &et_gpio_fops);
        cdev_add(&et_gpio_devs[i].cdev, et_gpio_devs[i].devno, 1);
    }

    et_gpio_class = class_create(THIS_MODULE, ET_GPIO_NAME);
    if(IS_ERR(et_gpio_class))
		return PTR_ERR(et_gpio_class);
    for (i = 0; i < GPIO_DEV_COUNT; i++) {
        et_gpio_devs[i].device = device_create(et_gpio_class, NULL, et_gpio_devs[i].devno, NULL, "%s%d", ET_GPIO_NAME, i);
        if (IS_ERR(et_gpio_devs[i].device)) {
            return PTR_ERR(et_gpio_devs[i].device);
        }
        printk("device %s initial is ok!\n", et_gpio_devs[i].name);
    }

    et_gpio_devs[0].addr_base = (unsigned long)ioremap_wc(GPIO_DEV_BASE0, GPIO_DEV_SIZE);
    et_gpio_devs[0].ch_count = GPIO_DEV_CHCOUNT0;
    et_gpio_devs[0].ch_width[0] = GPIO_DEV_WIDTH0_1;
    et_gpio_devs[0].ch_width[1] = GPIO_DEV_WIDTH0_2;
    et_gpio_devs[1].addr_base = (unsigned long)ioremap_wc(GPIO_DEV_BASE1, GPIO_DEV_SIZE);
    et_gpio_devs[1].ch_count = GPIO_DEV_CHCOUNT1;
    et_gpio_devs[1].ch_width[0] = GPIO_DEV_WIDTH1_1;
    et_gpio_devs[1].ch_width[1] = GPIO_DEV_WIDTH1_2;
    
    return 0;
}
 
static void __exit et_gpio_exit(void)  
{
    int i;
    for(i = 0; i < GPIO_DEV_COUNT; i++) {
        device_destroy(et_gpio_class, et_gpio_devs[i].devno);
        cdev_del(&et_gpio_devs[i].cdev);
        unregister_chrdev_region(et_gpio_devs[i].devno, 1);
    }
    class_destroy(et_gpio_class);  
}  
 
module_init(et_gpio_init);  
module_exit(et_gpio_exit);  
  
MODULE_AUTHOR("Kiyun");  
MODULE_ALIAS("et_gpio");  
MODULE_DESCRIPTION("driver for et_gpio");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  