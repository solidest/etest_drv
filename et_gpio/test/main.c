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
#define IOC_POLL_START _IOW(IOC_MAGIC_DIO, 3, long)
#define IOC_POLL_READ _IOR(IOC_MAGIC_DIO, 4, long)
#define IOC_POLL_STOP _IO(IOC_MAGIC_DIO, 5)


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
    struct et_gpio_data dirs0, dirs1,dirs2,dirs3;
    dirs0.value = 0x1FFFF;
    dirs0.mask = FULL_WIDTH;
    dirs1.value = 0x1FFFF;
    dirs1.mask = FULL_WIDTH;
    dirs2.value = 0x1FFFF;
    dirs2.mask = FULL_WIDTH;
    dirs3.value = 0x1FFFF;
    dirs3.mask = FULL_WIDTH;
    ioctl(fd0, IOC_SET_DIR, dirs0);
    ioctl(fd1, IOC_SET_DIR, dirs1);
    ioctl(fd2, IOC_SET_DIR, dirs2);
    ioctl(fd3, IOC_SET_DIR, dirs3);
    sleep(1);

    struct et_gpio_data data1, data2, data3, data4;
    int i = 0;
    while (i++<100)
    {
        read(fd0, &data1, sizeof(struct et_gpio_data));
        read(fd1, &data2, sizeof(struct et_gpio_data));
        read(fd2, &data3, sizeof(struct et_gpio_data));
        read(fd3, &data4, sizeof(struct et_gpio_data));
        printf("value0: %08X, mask0: %08X\n", data1.value, data1.mask);
        printf("value1: %08X, mask1: %08X\n", data2.value, data2.mask);
        printf("value2: %08X, mask2: %08X\n", data3.value, data3.mask);
        printf("value3: %08X, mask3: %08X\n", data4.value, data4.mask);
        sleep(1);
    }
}

int test_write(int fd0, int fd1, int fd2, int fd3)
{
    printf("write测试\n");

    struct et_gpio_data dirs0, dirs1,dirs2,dirs3;
    dirs0.value = 0x1FFFF;
    dirs0.mask = FULL_WIDTH;
    dirs1.value = 0x0;
    dirs1.mask = FULL_WIDTH;
    dirs2.value = 0x1FFFF;
    dirs2.mask = FULL_WIDTH;
    dirs3.value = 0x0;
    dirs3.mask = FULL_WIDTH;
    ioctl(fd0, IOC_SET_DIR, dirs0);
    ioctl(fd1, IOC_SET_DIR, dirs1);
    ioctl(fd2, IOC_SET_DIR, dirs2);
    ioctl(fd3, IOC_SET_DIR, dirs3);
    sleep(1);

    struct et_gpio_data data;
    data.value = 0x01;
    data.mask = FULL_WIDTH;
    write(fd1, &data, sizeof(struct et_gpio_data));
    write(fd0, &data, sizeof(struct et_gpio_data));
    write(fd2, &data, sizeof(struct et_gpio_data));
    write(fd3, &data, sizeof(struct et_gpio_data));    
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

void call_test_read()
{
        printf("-----------test_read---------\n");
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
}
void call_test_open_close()
{
    printf("-----------test_open_close---------\n");
    printf("\n");
    if(test_open_close()) {
        //fail++;
    }
}
void call_test_write()
{
    printf("-----------test_write---------\n");
    int fd0 = open("/dev/et_gpio0", O_RDWR);
    int fd1 = open("/dev/et_gpio1", O_RDWR);
    int fd2 = open("/dev/et_gpio2", O_RDWR);
    int fd3 = open("/dev/et_gpio3", O_RDWR);
    // int fd0 = open("/dev/et_gpio0", O_RDWR);
    // int fd1 = open("/dev/et_gpio1", O_RDWR);
    // test_write(fd0, fd1);
    // sleep(1);
    test_write(fd0, fd1, fd2, fd3);
    int i=0;
    while(++i<100)
    {
        sleep(1);
    }
    close(fd0);
    close(fd1);
    close(fd2);
    close(fd3);
}
void call_test_dir()
{
    int fd;
    fd = open("/dev/et_gpio0", O_RDWR);
    if(fd < 0) {
        printf("%s: 打开设备失败!!!\n", "/dev/et_gpio0");
        //return -1;
    }

    if(test_dir(fd)) {
        //fail++;
    }
    if(test_value(fd)) {
        // fail++;
    }
    close(fd);
    // if(fail) {
    //     printf("%d个测试失败\n", fail);
    //     return -1;
    // } else {
    //     printf("自动测试通过\n");
    // }
}

int test_poll_write(int fd1)
{
    printf("poll_write测试\n");

    struct et_gpio_data dirs0, dirs1,dirs2,dirs3;

    dirs1.value = 0x0;
    dirs1.mask = FULL_WIDTH;

    ioctl(fd1, IOC_SET_DIR, dirs1);

    sleep(1);

    struct et_gpio_data data;
    data.value = 0x015555;
    data.mask = FULL_WIDTH;
    write(fd1, &data, sizeof(struct et_gpio_data));
}
int call_test_poll_write()
{
    printf("-----------call_test_poll_write---------\n");
    int fd1 = open("/dev/et_gpio1", O_RDWR);
    test_poll_write(fd1);
    int i=0;
    while(++i<200)
    {
        sleep(1);
    }
    close(fd1);
}

int test_poll()
{
    int fd = open("/dev/et_gpio0", O_RDWR | O_NONBLOCK);
    struct et_gpio_data dirs;
    dirs.value = 0xFFFFFFFF;
    dirs.mask = FULL_WIDTH;
    ioctl(fd, IOC_SET_DIR, dirs);
    printf("%d\n", __LINE__);

    int epfd = epoll_create(10);
    struct epoll_event evs = {
        .events = EPOLLIN,
    };
    int res = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evs);
    printf("%d\n", __LINE__);
    ioctl(fd, IOC_POLL_START, 1000000000);
    printf("%d\n", __LINE__);
    sleep(3);
    close(res);
    close(fd);
    return 0;

    while (1)
    {
        struct et_gpio_data data;
        printf("%d\n", __LINE__);
        int evcount = 0; //epoll_wait(epfd, &evs, 1, 0);
        sleep(1);
        printf("%d\n", __LINE__);
        ioctl(fd, IOC_POLL_READ, &data);
        printf("evcount: %d, value: %08X, mask: %08X\n", evcount, data.value, data.mask);
    }
   
    return 0;
}

int main(int argc, char **argv)
{
    printf("a:test_read\n");
    printf("b:test_write\n");
    printf("c:test_open_close\n");
    printf("d:test_dir\n");
    printf("e:test_poll\n");

    printf("Input char to test---------------\n");

    char c;
    c=getchar();
    
    int fail = 0;

    switch (c)
    {
        case 'a':
            call_test_read();
            break;
        case 'b':
            printf("-----------test_write-------------\n");
            call_test_write();
            break;
        case 'c':
            printf("-----------test_open_close-------------\n");
            call_test_open_close();
            break;
        case 'd':
            printf("-----------test_dir----------------\n");
            call_test_dir();
            break;
        case 'e':
            printf("-----------test_poll----------------\n");
            test_poll();
            break;

        default:
            break;
    }
    return 0;

    if(2 != argc) {
        printf("输入参数错误\n");
        return -1;
    }


   // if(test_poll(fd)) {
    //     fail++;
    // }
    

    // test_manual(argv[1]);

    return 0;
}
