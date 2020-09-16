/** ===================================================== **
 *Author : solidest
 *Website: http://www.kiyun.com
 *Created: 2020-9-16
 *Version: 1.0
 ** ===================================================== **/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define IOC_MAGIC_DIO 'd'

#define IOC_SET_DIR _IOW(IOC_MAGIC_DIO, 1, long)
#define IOC_GET_DIR _IOWR(IOC_MAGIC_DIO, 2, long)

struct et_gpio_dir{
	unsigned int ch_id;             //通道号
    unsigned int io_dir;            //io方向 按位 0为输入 1为输出
    unsigned int mask;              //引脚掩码 1需要设置的引脚
};

struct et_gpio_value{
	unsigned int ch_id;             //通道号
    unsigned int io_value;          //io值 按位 0为低电平 1为高电平
    unsigned int mask;              //引脚掩码 1为有效的引脚
};

int test_value(int fd)
{
    printf("%s : 开始测试\n", __func__);
    struct et_gpio_value v1 = {
        .ch_id = 0,
        .io_value = 1,
        .mask = 2,
    };
    ssize_t ws = write(fd, &v1, sizeof(struct et_gpio_value));
    printf("写入: %ld\n", ws);

    struct et_gpio_value v2 = {
        .ch_id = 1,
    };
    read(fd, &v2, sizeof(struct et_gpio_value));
    printf("读取结果： %d:%d:%d\n", v2.ch_id, v2.io_value, v2.mask);
    return 0;
}

int test_dir(int fd)
{
    printf("%s : 开始测试\n", __func__);
    struct et_gpio_dir dirs1 = {
        .ch_id = 0,
        .io_dir = 1,
        .mask = 2
    };

    ioctl(fd, IOC_SET_DIR, &dirs1);
    struct et_gpio_dir dirs2;
    ioctl(fd, IOC_GET_DIR, &dirs2);
    printf("测试结果: %d:%d:%d\n", dirs2.ch_id, dirs2.io_dir, dirs2.mask);
    return 0;
}

int main(int argc, char **argv)
{
    int fd;

    if(2 != argc) {
        printf("输入参数错误\n");
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if(fd < 0) {
        printf("%s: 打开设备失败!!!\n", argv[1]);
        return -1;
    }

    int fail = 0;
    if(test_dir(fd)) {
        fail++;
    }
    if(test_value(fd)) {
        fail++;
    }
    
    close(fd);
    if(fail) {
        printf("%d个测试失败", fail);
    }
    return 0;
}
