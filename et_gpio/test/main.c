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
#include <sys/epoll.h> 

#define IOC_MAGIC_DIO 'd'
#define IOC_SET_DIR _IOW(IOC_MAGIC_DIO, 1, long)
#define IOC_GET_DIR _IOR(IOC_MAGIC_DIO, 2, long)
#define IOC_POLL_RESET _IO(IOC_MAGIC_DIO, 3)
#define IOC_POLL_READ _IOR(IOC_MAGIC_DIO, 4, long)


#define CHANNEL_WIDTH   17
#define FULL_WIDTH 0x1FFFF

struct et_gpio_data {
    unsigned int value;
    unsigned int mask;
};

int test_value(int fd)
{
    printf("开始测试: %s\n", __func__);
    struct et_gpio_data dirs;
    struct et_gpio_data data1, data2;

    dirs.value = 0x15555;
    dirs.mask = FULL_WIDTH;
    ioctl(fd, IOC_SET_DIR, dirs);

    data1.value = 0xFFFFFFFF;
    data1.mask = 0xFFFFFFFF;
    write(fd, &data1, sizeof(struct et_gpio_data));
    sleep(1);
    read(fd, &data2, sizeof(struct et_gpio_data));
    if(data2.value!=((~dirs.value) & data1.value & FULL_WIDTH) || data2.mask!=dirs.value) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %08X, mask: %08X\n", (~dirs.value) & data1.value & FULL_WIDTH, data2.mask);
        return -1;
    }
    return 0;
}


int test_dir(int fd)
{
    printf("开始测试: %s\n", __func__);
    struct et_gpio_data dirs1, dirs2;
    
    dirs1.value = 0;
    dirs1.mask = FULL_WIDTH,
    ioctl(fd, IOC_SET_DIR, dirs1);
    ioctl(fd, IOC_GET_DIR, &dirs2);
    if((dirs2.value&FULL_WIDTH) != 0 || dirs2.mask != FULL_WIDTH) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %d, mask: %d\n", dirs2.value, dirs2.mask);
        return -1;
    }

    dirs1.value = 0xFFFFFFFF;
    dirs1.mask = 0xFFFFFFFF,
    ioctl(fd, IOC_SET_DIR, dirs1);
    ioctl(fd, IOC_GET_DIR, &dirs2);
    if((dirs2.value&FULL_WIDTH) != FULL_WIDTH || dirs2.mask != FULL_WIDTH) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %x, mask: %x\n", dirs2.value, dirs2.mask);
        return -1;
    }

    dirs1.value = 0x0;
    dirs1.mask = 0x00000001 << 8;
    ioctl(fd, IOC_SET_DIR, dirs1);
    ioctl(fd, IOC_GET_DIR, &dirs2);
    if((dirs2.value&FULL_WIDTH) != (unsigned int)0x1FEFF || dirs2.mask != FULL_WIDTH) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %x, mask: %x\n", dirs2.value, dirs2.mask);
        return -1;
    }

    dirs1.value = 0x0;
    dirs1.mask = 0x03;
    ioctl(fd, IOC_SET_DIR, dirs1);
    ioctl(fd, IOC_GET_DIR, &dirs2);
    if((dirs2.value&FULL_WIDTH) != 0x1FEFC || dirs2.mask != FULL_WIDTH) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %x, mask: %x\n", dirs2.value, dirs2.mask);
        return -1;
    }

    dirs1.value = 0x2;
    dirs1.mask = 0x02;
    ioctl(fd, IOC_SET_DIR, dirs1);
    ioctl(fd, IOC_GET_DIR, &dirs2);
    if((dirs2.value&FULL_WIDTH) != 0x1FEFE || dirs2.mask != FULL_WIDTH) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %x, mask: %x\n", dirs2.value, dirs2.mask);
        return -1;
    }
    return 0;
}

int test_poll(int fd)
{
    printf("开始测试: %s\n", __func__);

    struct et_gpio_data dirs;
    dirs.value = 0x15555;
    dirs.mask = FULL_WIDTH;
    ioctl(fd, IOC_SET_DIR, dirs);

    int epfd = epoll_create(10);
    struct epoll_event evs = {
        .events = EPOLLIN,
    };
    int res = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evs);
    ioctl(fd, IOC_POLL_RESET);
    int evcount1 = epoll_wait(epfd, &evs, 1, 0);

    struct et_gpio_data data;
    data.value = 0xFF667755;
    data.mask = 0x55667788;
    write(fd, &data, sizeof(struct et_gpio_data));
    ioctl(fd, IOC_POLL_READ, &data);
    int evcount2 = epoll_wait(epfd, &evs, 1, 0);
    if(evcount1!=0 || evcount2!=0 || data.value!=0 || data.mask!=((~dirs.value)&(FULL_WIDTH))) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %x, mask: %x\n", data.value, data.mask);
        return -1;
    }
    return 0;
}


void test_manual(char* file)
{
    printf("手动测试\n");
    int fd = open(file, O_RDWR);
    struct et_gpio_data dirs;
    struct et_gpio_data data;

    dirs.value = 0x100FF;
    dirs.mask = FULL_WIDTH;
    ioctl(fd, IOC_SET_DIR, dirs);

    data.value = 0x00FF;
    data.mask = 0xFFFF;
    write(fd, &data, sizeof(struct et_gpio_data));

    ioctl(fd, IOC_POLL_RESET);
    int epfd = epoll_create(10);
    struct epoll_event evs = {
        .events = EPOLLIN,
    };
    int res = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evs);
    int ev_count = -2;

    int count = 0;
    while (1)
    {
        count++;
        sleep(1);

        if(ev_count==-2) {
            read(fd, &data, sizeof(struct et_gpio_data));
            printf("value: %05X, mask: %05X\n", data.value, data.mask);
        } else if(ev_count>0) {
            ioctl(fd, IOC_POLL_READ, &data);
            printf("value: %05X, mask: %05X, event: %d\n", data.value, data.mask, ev_count);
        }
        ev_count = epoll_wait(epfd, &evs, 1, 0);

        data.value = count%2==0 ? 0x10000 : 0x0;
        data.mask = 0x10000;
        write(fd, &data, sizeof(struct et_gpio_data));
    }
}


int test_demo()
{
    printf("demo测试\n");
    struct et_gpio_data data1, data2;

    int fd2 = open("/dev/et_gpio0", O_RDWR);
    int fd1 = open("/dev/et_gpio1", O_RDWR);

    data1.value = 0x100FF;
    data1.mask = FULL_WIDTH;
    write(fd2, &data1, sizeof(struct et_gpio_data));
    sleep(1);
    read(fd1, &data2, sizeof(struct et_gpio_data));
    printf("value: %08X, mask: %08X\n", data2.value, data2.mask);
    close(fd1);
    close(fd2);
}

int test_read(int fd0, int fd1, int fd2, int fd3)
{
    printf("read测试\n");
    struct et_gpio_data data1, data2, data3, data4;
    int i = 0;
    while (i++<10)
    {
        read(fd0, &data1, sizeof(struct et_gpio_data));
        read(fd1, &data2, sizeof(struct et_gpio_data));
        read(fd0, &data3, sizeof(struct et_gpio_data));
        read(fd1, &data4, sizeof(struct et_gpio_data));
        printf("value0: %08X, mask0: %08X\n", data1.value, data1.mask);
        printf("value1: %08X, mask1: %08X\n", data2.value, data2.mask);
        printf("value2: %08X, mask0: %08X\n", data3.value, data3.mask);
        printf("value3: %08X, mask1: %08X\n", data4.value, data4.mask);
        sleep(1);
    }
}

int test_write(int fd0, int fd1)
{
    printf("write测试\n");

    struct et_gpio_data dirs0, dirs1;
    dirs0.value = 0x1FFFF;
    dirs0.mask = FULL_WIDTH;
    dirs1.value = 0x0;
    dirs1.mask = FULL_WIDTH;
    ioctl(fd0, IOC_SET_DIR, dirs0);
    ioctl(fd1, IOC_SET_DIR, dirs1);
    sleep(1);

    struct et_gpio_data data;
    data.value = 0x01;
    data.mask = FULL_WIDTH;
    write(fd1, &data, sizeof(struct et_gpio_data));
    write(fd0, &data, sizeof(struct et_gpio_data));
}

int test_open_close()
{
    printf("开始测试%s\n", __func__);

    int fd0 = open("/dev/et_gpio0", O_RDWR);
    int fd1 = open("/dev/et_gpio1", O_RDWR);
    struct et_gpio_data dirs0, dirs1;
    dirs0.value = 0x2;
    dirs0.mask = FULL_WIDTH;
    dirs1.value = 0x01;
    dirs0.mask = FULL_WIDTH;
    ioctl(fd0, IOC_SET_DIR, dirs0);
    ioctl(fd1, IOC_SET_DIR, dirs1);
    close(fd0);
    close(fd1);

    struct et_gpio_data data0, data1;
    fd0 = open("/dev/et_gpio0", O_RDWR);
    fd1 = open("/dev/et_gpio1", O_RDWR);
    read(fd0, &data0, sizeof(struct et_gpio_data));
    read(fd1, &data1, sizeof(struct et_gpio_data));
    close(fd0);
    close(fd1);

    if(data0.value!=0 || data0.mask!=0x1FFFF) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %x, mask: %x\n", data0.value, data0.mask);
        return -1;
    }

    if(data1.value!=0 || data1.mask!=0x1FFFF) {
        printf("ERROR %s : %d\n", __func__, __LINE__);
        printf("value: %x, mask: %x\n", data1.value, data1.mask);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int fd0 = open("/dev/et_gpio0", O_RDONLY);
    int fd1 = open("/dev/et_gpio1", O_RDONLY);
    int fd2 = open("/dev/et_gpio2", O_RDONLY);
    int fd3 = open("/dev/et_gpio3", O_RDONLY);
    // int fd0 = open("/dev/et_gpio0", O_RDWR);
    // int fd1 = open("/dev/et_gpio1", O_RDWR);
    // test_write(fd0, fd1);
    // sleep(1);
    test_read(fd0, fd1, fd2, fd3);
    close(fd0);
    close(fd1);
    close(fd2);
    close(fd3);
    return 0;

    int fd;
    int fail = 0;

    if(2 != argc) {
        printf("输入参数错误\n");
        return -1;
    }

    printf("\n");
    if(test_open_close()) {
        fail++;
    }

    fd = open(argv[1], O_RDWR);
    if(fd < 0) {
        printf("%s: 打开设备失败!!!\n", argv[1]);
        return -1;
    }


    if(test_dir(fd)) {
        fail++;
    }
    if(test_value(fd)) {
        fail++;
    }
    // if(test_poll(fd)) {
    //     fail++;
    // }
    
    close(fd);
    if(fail) {
        printf("%d个测试失败\n", fail);
        return -1;
    } else {
        printf("自动测试通过\n");
    }

    // test_manual(argv[1]);

    return 0;
}
