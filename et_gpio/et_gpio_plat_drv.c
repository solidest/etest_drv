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

#define ET_DEBUG

//TODO 移除此处的定义，改为从资源数据获取或从设备树获取
#define ET_GPIO_NAME        "et_gpio"
#define GPIO_DEV_COUNT      4
#define GPIO_DEV_BASEA      0x80050000
#define GPIO_DEV_BASEB      0x80060000
#define GPIO_DEV_SIZE       0x10000
#define GPIO_DEV_WIDTH0     17
#define GPIO_DEV_WIDTH1     17
#define GPIO_DEV_WIDTH2     17
#define GPIO_DEV_WIDTH3     17

#define IOC_MAGIC_DIO 'd'
#define IOC_SET_DIR _IOW(IOC_MAGIC_DIO, 1, long)
#define IOC_GET_DIR _IOR(IOC_MAGIC_DIO, 2, long)
#define IOC_POLL_START _IOW(IOC_MAGIC_DIO, 3, long)
#define IOC_POLL_GET _IOR(IOC_MAGIC_DIO, 4, long)
#define IOC_POLL_STOP _IO(IOC_MAGIC_DIO, 5)

struct et_gpio_data {
    unsigned int value;
    unsigned int mask;
};

struct et_gpio_dev {
    char name[16];
    dev_t devno;
    struct cdev cdev;
    struct device *device;
    unsigned int width;
    unsigned int * p_value;
    unsigned int * p_state;
    spinlock_t h_lock;
    #ifdef ET_DEBUG
    unsigned int demo_value;
    unsigned int demo_state;
    #endif
};

struct et_gpio_handle {
    struct et_gpio_dev* dev;
    unsigned int used_pin;
    struct et_gpio_data poll_data;
    wait_queue_head_t h_poll;
    struct hrtimer timer;
    unsigned long timer_step;
};

static int _poll_stop(struct et_gpio_handle* h);
static enum hrtimer_restart _do_poll(struct hrtimer *hrt);
static int _poll_stop(struct et_gpio_handle* hdl);

/**
 * IO操作
*/

static struct class * et_gpio_class;  
static struct et_gpio_dev *et_gpio_devs;

static int et_gpio_open(struct inode * inode_p, struct file * file_p)
{
    struct et_gpio_dev * dev;
    struct et_gpio_handle* hld;
    
    dev = container_of(inode_p->i_cdev, struct et_gpio_dev, cdev);
    hld = (struct et_gpio_handle*)kmalloc(sizeof(struct et_gpio_handle), GFP_KERNEL);
    if(IS_ERR(hld))
		return PTR_ERR(hld);

    hld->dev = dev;
    hld->used_pin = 0;
    hld->timer_step = 0;
    hld->poll_data.value = 0;
    hld->poll_data.mask = 0;
    init_waitqueue_head(&hld->h_poll);
    hrtimer_init(&hld->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hld->timer.function = _do_poll;

    file_p->private_data = hld;
    return 0;
}

static int et_gpio_close(struct inode * inode_p, struct file * file_p)
{
    struct et_gpio_handle* hld = (struct et_gpio_handle*)file_p->private_data;
    _poll_stop(hld);
    kfree(hld);
    file_p->private_data = NULL;
    return 0;
}

static ssize_t et_gpio_write(struct file *file_p, const char __user *buf, size_t len, loff_t *loff_t_p)
{
    struct et_gpio_data data;
    unsigned int value, state, mask, pos, i;
    struct et_gpio_handle * hdl = (struct et_gpio_handle *)file_p->private_data;
    struct et_gpio_dev * dev = hdl->dev;

    if(len != sizeof(struct et_gpio_data)) {
        return -EINVAL;
    }

    spin_lock(&dev->h_lock);
    value = ioread32(dev->p_value);
    state = ioread32(dev->p_state);
    copy_from_user(&data, buf, len);
    mask = hdl->used_pin & data.mask & (~state);
    if(mask == 0) {
        spin_unlock(&dev->h_lock); 
        return 0;
    }
    for(i = 0; i < dev->width; i++) {
        pos = BIT(i);
        if(mask & pos) {
            if (data.value & pos)
                value |= pos;
            else
                value &= ~pos;
        }
    }
    iowrite32(value, dev->p_value);
    spin_unlock(&dev->h_lock); 

    return len;
}

static ssize_t et_gpio_read(struct file *file_p, char __user *buf, size_t len, loff_t *loff_t_p)
{
    struct et_gpio_data data;
    struct et_gpio_handle * hdl = (struct et_gpio_handle *)file_p->private_data;
    struct et_gpio_dev * dev = hdl->dev;

    if(len != sizeof(struct et_gpio_data)) {
        return -EINVAL;
    }
    spin_lock(&dev->h_lock);

    data.value = ioread32(dev->p_value) & hdl->used_pin;
    data.mask = ioread32(dev->p_state) & hdl->used_pin;
    copy_to_user(buf, &data, len);

    spin_unlock(&dev->h_lock); 
    return len;
}

/**
 * POLL操作
*/

unsigned int et_gpio_poll(struct file *file_p, struct poll_table_struct *wait)
{
    struct et_gpio_handle * hdl = (struct et_gpio_handle *)file_p->private_data;
    struct et_gpio_dev * dev = hdl->dev;
    unsigned int ret = 0;
    spin_lock(&dev->h_lock);

    poll_wait(file_p, &hdl->h_poll, wait);
    if(hdl->poll_data.mask) {
        ret = POLLPRI;
    }

    spin_unlock(&dev->h_lock);
    return ret;
}

static enum hrtimer_restart _do_poll(struct hrtimer *hrt)
{
    unsigned int value, mask;
    struct et_gpio_handle * hdl = container_of(hrt, struct et_gpio_handle, timer);
    struct et_gpio_dev * dev = hdl->dev;
    spin_lock(&dev->h_lock);

    if(hdl->poll_data.mask) {
        wake_up_interruptible(&hdl->h_poll);
        goto EXIT;
    }

    value = ioread32(dev->p_value) & ioread32(dev->p_state) & hdl->used_pin;
    mask = (value ^ hdl->poll_data.value);
    #ifdef ET_DEBUG
    hdl->poll_data.value = 1;
    hdl->poll_data.mask = 1; 
    wake_up_interruptible(&hdl->h_poll);
    #else
    if(mask) {
        hdl->poll_data.value = value;
        hdl->poll_data.mask = mask;
        wake_up_interruptible(&hdl->h_poll);
    }
    #endif

EXIT:
    spin_unlock(&dev->h_lock);
    hrtimer_forward(hrt, hrt->base->get_time(), ktime_set(0, hdl->timer_step));
    return HRTIMER_RESTART;
}

static int _poll_start(struct et_gpio_handle* hdl, unsigned long tick_us)
{
    struct et_gpio_dev* dev = hdl->dev;
    spin_lock(&dev->h_lock);

    if(hdl->timer_step != 0) {
        spin_unlock(&dev->h_lock);
        return -1;
    }
    if(tick_us>1000) {
        tick_us = 1000;
    } else if(tick_us<1) {
        tick_us = 1;
    }
    hdl->timer_step = tick_us * 1000;
    hdl->poll_data.mask = 0x0;
    hdl->poll_data.value = ioread32(dev->p_value) & ioread32(dev->p_state) & hdl->used_pin;

    spin_unlock(&dev->h_lock);
    hrtimer_start(&hdl->timer, ktime_set(0, hdl->timer_step), HRTIMER_MODE_REL);
    return 0;
};

static int _poll_stop(struct et_gpio_handle* hdl)
{
    struct et_gpio_dev* dev = hdl->dev;
    spin_lock(&dev->h_lock);

    if(hdl->timer_step != 0) {
        hrtimer_cancel(&hdl->timer);
        hdl->timer_step = 0;
    }

    spin_unlock(&dev->h_lock);
    return 0;
};

/**
 * IOCTL操作
*/

static long et_gpio_ioctl(struct file *file_p, unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    struct et_gpio_handle * hdl = (struct et_gpio_handle *)file_p->private_data;
    struct et_gpio_dev * dev = hdl->dev;

    if(_IOC_TYPE(cmd) != IOC_MAGIC_DIO) return -ENOTTY;

    switch (cmd) {
        case IOC_SET_DIR: {
            struct et_gpio_data * data = (struct et_gpio_data *)&arg;
            int i;
            unsigned int state, pos;
            spin_lock(&dev->h_lock);
            hdl->used_pin = data->mask & (0xFFFFFFFF >> (32-dev->width));
            state = ioread32(dev->p_state);
            for (i = 0; i < dev->width; i++) {
                pos = BIT(i);
                if (data->mask & pos) {
                    if (data->value & pos)
                        state |= pos;
                    else
                        state &= ~pos;
                }
            }
            iowrite32(state, dev->p_state);
            spin_unlock(&dev->h_lock);
            break;            
        }

        case IOC_GET_DIR: {
            struct et_gpio_data res;
            spin_lock(&dev->h_lock);
            res.mask = hdl->used_pin;
            res.value = ioread32(dev->p_state) & res.mask;
            copy_to_user((void*)arg, &res, sizeof(struct et_gpio_data));
            spin_unlock(&dev->h_lock);
            break;
        }

        case IOC_POLL_START: {
            return _poll_start(hdl, arg);
        }

        case IOC_POLL_STOP:{
            return _poll_stop(hdl);
        }

        case IOC_POLL_GET: {
            spin_lock(&dev->h_lock);
            copy_to_user((void*)arg, &(hdl->poll_data), sizeof(struct et_gpio_data));
            hdl->poll_data.mask = 0;
            spin_unlock(&dev->h_lock);
            break;
        }

        default:
            retval = -EINVAL;
            break;
    }

    return retval;
}

/**
 * 注册与注销
*/

static struct file_operations et_gpio_fops = {
    .owner = THIS_MODULE,
    .open  = et_gpio_open,
    .release = et_gpio_close,
    .write = et_gpio_write,
    .read = et_gpio_read,
    .unlocked_ioctl = et_gpio_ioctl,
    .poll = et_gpio_poll,
};

static int et_gpio_probe(struct platform_device * pdev)
{  
    int i;
    dev_t devno;
    int err;

    et_gpio_devs = (struct et_gpio_dev*)kmalloc(sizeof(struct et_gpio_dev)*GPIO_DEV_COUNT, GFP_KERNEL);
    if(IS_ERR(et_gpio_devs))
		return PTR_ERR(et_gpio_devs);
    et_gpio_class = class_create(THIS_MODULE, ET_GPIO_NAME);
    if(IS_ERR(et_gpio_class))
		return PTR_ERR(et_gpio_class);
    err = alloc_chrdev_region(&devno, 0, GPIO_DEV_COUNT, ET_GPIO_NAME);
    if(err)
        return err;

    for (i = 0; i < GPIO_DEV_COUNT; i++) {
        et_gpio_devs[i].devno = devno + i;
        sprintf(et_gpio_devs[i].name, "%s%d", ET_GPIO_NAME,  MINOR(et_gpio_devs[i].devno));
        et_gpio_devs[i].cdev.owner = THIS_MODULE;
        cdev_init(&et_gpio_devs[i].cdev, &et_gpio_fops);
        cdev_add(&et_gpio_devs[i].cdev, et_gpio_devs[i].devno, 1);
        et_gpio_devs[i].device = device_create(et_gpio_class, NULL, et_gpio_devs[i].devno, NULL, "%s%d", ET_GPIO_NAME, i);
        if(IS_ERR(et_gpio_devs[i].device)) {
            return PTR_ERR(et_gpio_devs[i].device);
        }
        spin_lock_init(&et_gpio_devs[i].h_lock);

        printk("device %s initial is ok!\n", et_gpio_devs[i].name);
    }

    #ifdef ET_DEBUG
    et_gpio_devs[0].demo_value = 0x0;
    et_gpio_devs[0].demo_state = 0xFFFFFFFF>>(32-GPIO_DEV_WIDTH0);
    et_gpio_devs[0].p_value = &et_gpio_devs[0].demo_value;
    et_gpio_devs[0].p_state = &et_gpio_devs[0].demo_state;
    et_gpio_devs[0].width = GPIO_DEV_WIDTH0;
    
    et_gpio_devs[1].demo_value = 0x0;
    et_gpio_devs[1].demo_state = 0xFFFFFFFF>>(32-GPIO_DEV_WIDTH1);
    et_gpio_devs[1].p_value = &et_gpio_devs[1].demo_value;
    et_gpio_devs[1].p_state = &et_gpio_devs[1].demo_state;
    et_gpio_devs[1].width = GPIO_DEV_WIDTH1;

    et_gpio_devs[2].demo_value = 0x0;
    et_gpio_devs[2].demo_state = 0xFFFFFFFF>>(32-GPIO_DEV_WIDTH2);
    et_gpio_devs[2].p_value = &et_gpio_devs[2].demo_value;
    et_gpio_devs[2].p_state = &et_gpio_devs[2].demo_state;
    et_gpio_devs[2].width = GPIO_DEV_WIDTH2;

    et_gpio_devs[3].demo_value = 0x0;
    et_gpio_devs[3].demo_state = 0xFFFFFFFF>>(32-GPIO_DEV_WIDTH3);
    et_gpio_devs[3].p_value = &et_gpio_devs[3].demo_value;
    et_gpio_devs[3].p_state = &et_gpio_devs[3].demo_state;
    et_gpio_devs[3].width = GPIO_DEV_WIDTH3;
    #else
    // #define GPIO_CH_OFFSET      0x08
    // #define GPIO_STATE_OFFSET   0x04
    et_gpio_devs[0].p_value = (unsigned int*)ioremap_wc(GPIO_DEV_BASEA, GPIO_DEV_SIZE);
    et_gpio_devs[0].p_state = et_gpio_devs[0].p_value + 1;
    et_gpio_devs[0].width = GPIO_DEV_WIDTH0;

    et_gpio_devs[1].p_value = et_gpio_devs[0].p_value + 2;
    et_gpio_devs[1].p_state = et_gpio_devs[0].p_value + 3;
    et_gpio_devs[1].width = GPIO_DEV_WIDTH1;

    et_gpio_devs[2].p_value = (unsigned int*)ioremap_wc(GPIO_DEV_BASEB, GPIO_DEV_SIZE);
    et_gpio_devs[2].p_state = et_gpio_devs[2].p_value + 1;
    et_gpio_devs[2].width = GPIO_DEV_WIDTH2;

    et_gpio_devs[3].p_value = et_gpio_devs[2].p_value + 2;
    et_gpio_devs[3].p_state = et_gpio_devs[2].p_value + 3;
    et_gpio_devs[3].width = GPIO_DEV_WIDTH3;
    #endif
    
    return 0;
}
 
static int et_gpio_remove(struct platform_device * dev)  
{
    int i;
    #ifndef ET_DEBUG
    iounmap(et_gpio_devs[0].p_value);
    iounmap(et_gpio_devs[2].p_value);
    #endif
    for(i = 0; i < GPIO_DEV_COUNT; i++) {
        device_destroy(et_gpio_class, et_gpio_devs[i].devno);
        cdev_del(&et_gpio_devs[i].cdev);
        unregister_chrdev_region(et_gpio_devs[i].devno, 1);
    }
    class_destroy(et_gpio_class);
    et_gpio_class = NULL;
    kfree(et_gpio_devs);
    et_gpio_devs = NULL;
    return 0;
}

static struct platform_driver et_gpio_driver = {
    .driver = {
        .name = "et-gpio",
        .owner = THIS_MODULE,
    },
    .probe = et_gpio_probe,
    .remove = et_gpio_remove,
};

module_platform_driver(et_gpio_driver);
  
MODULE_AUTHOR("Kiyun");  
MODULE_ALIAS("et_gpio");  
MODULE_DESCRIPTION("driver for et_gpio");  
MODULE_VERSION("v1.0");  
MODULE_LICENSE("GPL");  