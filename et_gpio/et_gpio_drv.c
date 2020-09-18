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
#include <linux/poll.h>
#include <asm/uaccess.h>

#define ET_DEBUG

#define ET_GPIO_NAME        "et_gpio"
#define ET_GPIO_MAJOR       300
#define GPIO_DEV_COUNT      4
#define GPIO_DEV_BASEA      0x80050000
#define GPIO_DEV_BASEB      0x80060000
#define GPIO_DEV_SIZE       0x10000
#define GPIO_CH_OFFSET      0x08
#define GPIO_STATE_OFFSET   0x04
#define GPIO_DEV_WIDTH0     17
#define GPIO_DEV_WIDTH1     17
#define GPIO_DEV_WIDTH2     17
#define GPIO_DEV_WIDTH3     17

#define IOC_MAGIC_DIO 'd'
#define IOC_SET_DIR _IOW(IOC_MAGIC_DIO, 1, long)
#define IOC_GET_DIR _IOR(IOC_MAGIC_DIO, 2, long)
#define IOC_POLL_RESET _IO(IOC_MAGIC_DIO, 3)
#define IOC_POLL_READ _IOR(IOC_MAGIC_DIO, 4, long)

struct et_gpio_data {
    unsigned int value;
    unsigned int mask;
};

struct et_gpio_dev {
    char name[16];
    dev_t devno;
    struct cdev cdev;
    struct device *device;
    unsigned long addr_base;
    unsigned int width;
    struct et_gpio_data poll_data;
    wait_queue_head_t h_wait;
    spinlock_t h_lock;
    #ifdef ET_DEBUG
    unsigned int demo_value;
    unsigned int demo_state;
    #endif
};



/////////////////////////////

static struct class * et_gpio_class;  
static struct et_gpio_dev et_gpio_devs[GPIO_DEV_COUNT];

static int et_gpio_open(struct inode * inode_p, struct file * file_p)
{
    struct et_gpio_dev * dev = container_of(inode_p->i_cdev, struct et_gpio_dev, cdev);
    if (IS_ERR(dev)) {
        return PTR_ERR(dev);
    }

    spin_lock(&dev->h_lock);
    file_p->private_data = dev;
    if(file_p->f_mode & FMODE_WRITE) {
        printk("device %s reset when open.\n", dev->name);
        writel(0x0, (void*)(dev->addr_base));
        writel(0xFFFFFFFF, (void*)(dev->addr_base + GPIO_STATE_OFFSET));
    }
    spin_unlock(&dev->h_lock);  
    return 0;
}

static int et_gpio_close(struct inode * inode_p, struct file * file_p)
{
    struct et_gpio_dev * dev = container_of(inode_p->i_cdev, struct et_gpio_dev, cdev);
    if (IS_ERR(dev)) {
        return PTR_ERR(dev);
    }

    spin_lock(&dev->h_lock);
    file_p->private_data = NULL;
    if(file_p->f_mode & FMODE_WRITE) {
        printk("device %s reset when close.\n", dev->name);
        writel(0x0, (void*)(dev->addr_base));
        writel(0xFFFFFFFF, (void*)(dev->addr_base + GPIO_STATE_OFFSET));        
    }
    spin_unlock(&dev->h_lock);  
    return 0;
}

static ssize_t et_gpio_write(struct file *file_p, const char __user *buf, size_t len, loff_t *loff_t_p)
{
    struct et_gpio_dev * dev = (struct et_gpio_dev *)file_p->private_data;
    struct et_gpio_data data;
    unsigned int value, state, pos, i;

    if(len != sizeof(struct et_gpio_data)) {
        return -1;
    }
    if (IS_ERR(dev))
        return PTR_ERR(dev);
    copy_from_user(&data, buf, len);

    spin_lock(&dev->h_lock);
    value = readl((void*)dev->addr_base);
    state = readl((void*)dev->addr_base + GPIO_STATE_OFFSET);
    for(i = 0; i < dev->width; i++) {
        if(data.mask == 0)
            break;
        pos = BIT(i);
        if((data.mask & pos) && !(state & pos)) {
            if (data.value & pos)
                value |= pos;
            else
                value &= ~pos;
        }
    }
    writel(value, (void*)(dev->addr_base));
    spin_unlock(&dev->h_lock); 
    return 0;
}

static ssize_t et_gpio_read(struct file *file_p, char __user *buf, size_t len, loff_t *loff_t_p)
{
    struct et_gpio_dev * dev = (struct et_gpio_dev *)file_p->private_data;
    struct et_gpio_data data;

    if(len != sizeof(struct et_gpio_data)) {
        return -1;
    }
    if (IS_ERR(dev)) {
        return PTR_ERR(dev);
    }

    spin_lock(&dev->h_lock);
    data.value = readl((void*)dev->addr_base);
    data.mask = readl((void*)dev->addr_base + GPIO_STATE_OFFSET);
    spin_unlock(&dev->h_lock);

    copy_to_user(buf, &data, len);
    return 0;
}

unsigned int et_gpio_poll(struct file *file, struct poll_table_struct *wait)
{
    struct et_gpio_dev * dev = (struct et_gpio_dev *)file->private_data;
    unsigned int ret = 0, value;
    // printk("%s : %d\n", __func__, __LINE__);

    if (IS_ERR(dev))
        return PTR_ERR(dev);
    
    spin_lock(&dev->h_lock);
    value = (*(unsigned int*)(dev->addr_base)) & dev->poll_data.mask;
    if(value != dev->poll_data.value) {
        ret = POLLIN;
        dev->poll_data.mask = (value ^ dev->poll_data.value);
        dev->poll_data.value = value;
    }
    spin_unlock(&dev->h_lock);
    return ret;
}


static long et_gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    struct et_gpio_dev * dev = file->private_data;

    if (IS_ERR(dev))
        return PTR_ERR(dev);

    switch (cmd) {
        case IOC_SET_DIR: {
            struct et_gpio_data * data = (struct et_gpio_data *)&arg;
            int i;
            unsigned int state, pos;
            spin_lock(&dev->h_lock);
            state = readl((void*)dev->addr_base + GPIO_STATE_OFFSET);
            for (i = 0; i < dev->width; i++) {
                if (data->mask == 0)
                    break;
                pos = BIT(i);
                if (data->mask & pos) {
                    if (data->value & pos)
                        state |= pos;
                    else
                        state &= ~pos;
                }
            }
            writel(state, (unsigned int*)(dev->addr_base + GPIO_STATE_OFFSET));
            spin_unlock(&dev->h_lock);
            break;            
        }

        case IOC_GET_DIR: {
            struct et_gpio_data res;
            spin_lock(&dev->h_lock);
            res.mask = (0xFFFFFFFF >> (32-dev->width));
            res.value = readl((void*)dev->addr_base + GPIO_STATE_OFFSET);
            spin_unlock(&dev->h_lock);
            copy_to_user((void*)arg, &res, sizeof(struct et_gpio_data));
            break;
        }

        case IOC_POLL_RESET: {
            spin_lock(&dev->h_lock);
            dev->poll_data.mask = (~(*(unsigned int*)(dev->addr_base + GPIO_STATE_OFFSET))) & (0xFFFFFFFF>>(32-dev->width));
            dev->poll_data.value = (*(unsigned int*)(dev->addr_base)) & dev->poll_data.mask;
            spin_unlock(&dev->h_lock);
            break;
        }

        case IOC_POLL_READ: {
            copy_to_user((void*)arg, &(dev->poll_data), sizeof(struct et_gpio_data));
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
    .poll = et_gpio_poll,
};

static int __init et_gpio_init(void)  
{  
    int i;
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
        spin_lock_init(&et_gpio_devs[i].h_lock);
        printk("device %s initial is ok!\n", et_gpio_devs[i].name);
    }

    #ifdef ET_DEBUG
    et_gpio_devs[0].demo_value = 0x0;
    et_gpio_devs[0].demo_state = 0xFFFFFFFF>>(32-GPIO_DEV_WIDTH0);
    et_gpio_devs[1].demo_value = 0x0;
    et_gpio_devs[1].demo_state = 0xFFFFFFFF>>(32-GPIO_DEV_WIDTH1);
    et_gpio_devs[2].demo_value = 0x0;
    et_gpio_devs[2].demo_state = 0xFFFFFFFF>>(32-GPIO_DEV_WIDTH2);
    et_gpio_devs[3].demo_value = 0x0;
    et_gpio_devs[3].demo_state = 0xFFFFFFFF>>(32-GPIO_DEV_WIDTH3);
    et_gpio_devs[0].addr_base = (unsigned long)&et_gpio_devs[0].demo_value;
    et_gpio_devs[0].width = GPIO_DEV_WIDTH0;
    et_gpio_devs[1].addr_base = (unsigned long)&et_gpio_devs[1].demo_value;
    et_gpio_devs[1].width = GPIO_DEV_WIDTH1;
    et_gpio_devs[2].addr_base = (unsigned long)&et_gpio_devs[2].demo_value;
    et_gpio_devs[2].width = GPIO_DEV_WIDTH2;
    et_gpio_devs[3].addr_base = (unsigned long)&et_gpio_devs[3].demo_value;
    et_gpio_devs[3].width = GPIO_DEV_WIDTH3;
    #else
    et_gpio_devs[0].addr_base = (unsigned long)ioremap_wc(GPIO_DEV_BASEA, GPIO_DEV_SIZE);
    et_gpio_devs[1].addr_base = et_gpio_devs[0].addr_base + GPIO_CH_OFFSET;
    et_gpio_devs[2].addr_base = (unsigned long)ioremap_wc(GPIO_DEV_BASEB, GPIO_DEV_SIZE);
    et_gpio_devs[3].addr_base = et_gpio_devs[2].addr_base + GPIO_CH_OFFSET;
    et_gpio_devs[0].width = GPIO_DEV_WIDTH0;
    et_gpio_devs[1].width = GPIO_DEV_WIDTH1;
    et_gpio_devs[2].width = GPIO_DEV_WIDTH2;
    et_gpio_devs[3].width = GPIO_DEV_WIDTH3;
    #endif
    
    return 0;
}
 
static void __exit et_gpio_exit(void)  
{
    int i;
    for(i = 0; i < GPIO_DEV_COUNT; i++) {
        #ifndef ET_DEBUG
        if(i==0 || i==2) {
            iounmap((volatile void *)et_gpio_devs[i].addr_base);
        }
        #endif
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