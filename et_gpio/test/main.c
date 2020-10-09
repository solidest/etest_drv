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
#define IOC_POLL_GET _IOR(IOC_MAGIC_DIO, 4, long)
#define IOC_POLL_STOP _IO(IOC_MAGIC_DIO, 5)


#define CHANNEL_WIDTH   17
#define FULL_WIDTH 0x1FFFF

struct et_gpio_data {
    unsigned int value;
    unsigned int mask;
};


void continue_press_c()
{
    printf("press \'c\' then continue.");
    char c = getchar();

    while (c != 'c') {
        c = getchar();
    }
}

int test_dir()
{
    struct et_gpio_data dirs1, dirs2;
    int fd0 = open("/dev/et_gpio0", O_RDWR);
    int fd1 = open("/dev/et_gpio1", O_RDWR);
    int fd2 = open("/dev/et_gpio2", O_RDWR);
    int fd3 = open("/dev/et_gpio3", O_RDWR);

    for (int i = 0; i < CHANNEL_WIDTH; i++) {
        dirs1.value = 0xFFFFFFFF >> (32-i);
        dirs1.mask = 0xFFFFFFFF >> (32-i);
        ioctl(fd0, IOC_SET_DIR, dirs1);
        ioctl(fd1, IOC_SET_DIR, dirs1);
        ioctl(fd2, IOC_SET_DIR, dirs1);
        ioctl(fd3, IOC_SET_DIR, dirs1);
        sleep(1);

        ioctl(fd0, IOC_GET_DIR, &dirs2);
        printf("SET DIR value0=%08X, mask0=%08X\n", dirs1.value, dirs1.mask);
        printf("GET DIR value0=%08X, mask0=%08X\n", dirs2.value, dirs2.mask);
        ioctl(fd1, IOC_GET_DIR, &dirs2);
        printf("SET DIR value1=%08X, mask1=%08X\n", dirs1.value, dirs1.mask);
        printf("GET DIR value1=%08X, mask1=%08X\n", dirs2.value, dirs2.mask);
        ioctl(fd2, IOC_GET_DIR, &dirs2);
        printf("SET DIR value2=%08X, mask2=%08X\n", dirs1.value, dirs1.mask);
        printf("GET DIR value2=%08X, mask2=%08X\n", dirs2.value, dirs2.mask);
        ioctl(fd3, IOC_GET_DIR, &dirs2);
        printf("SET DIR value3=%08X, mask3=%08X\n", dirs1.value, dirs1.mask);
        printf("GET DIR value3=%08X, mask3=%08X\n", dirs2.value, dirs2.mask);
    }

    for (int i = 0; i < CHANNEL_WIDTH; i++) {
        dirs1.value = 0x0;
        dirs1.mask = 0xFFFFFFFF >> (32-i);
        ioctl(fd0, IOC_SET_DIR, dirs1);
        ioctl(fd1, IOC_SET_DIR, dirs1);
        ioctl(fd2, IOC_SET_DIR, dirs1);
        ioctl(fd3, IOC_SET_DIR, dirs1);
        sleep(1);

        ioctl(fd0, IOC_GET_DIR, &dirs2);
        printf("SET DIR value0=%08X, mask0=%08X\n", dirs1.value, dirs1.mask);
        printf("GET DIR value0=%08X, mask0=%08X\n", dirs2.value, dirs2.mask);
        ioctl(fd1, IOC_GET_DIR, &dirs2);
        printf("SET DIR value1=%08X, mask1=%08X\n", dirs1.value, dirs1.mask);
        printf("GET DIR value1=%08X, mask1=%08X\n", dirs2.value, dirs2.mask);
        ioctl(fd2, IOC_GET_DIR, &dirs2);
        printf("SET DIR value2=%08X, mask2=%08X\n", dirs1.value, dirs1.mask);
        printf("GET DIR value2=%08X, mask2=%08X\n", dirs2.value, dirs2.mask);
        ioctl(fd3, IOC_GET_DIR, &dirs2);
        printf("SET DIR value3=%08X, mask3=%08X\n", dirs1.value, dirs1.mask);
        printf("GET DIR value3=%08X, mask3=%08X\n", dirs2.value, dirs2.mask);
    }

    dirs1.value = 0x55555555 & FULL_WIDTH;
    dirs1.mask = 0x55555555;
    ioctl(fd0, IOC_SET_DIR, dirs1);
    ioctl(fd1, IOC_SET_DIR, dirs1);
    ioctl(fd2, IOC_SET_DIR, dirs1);
    ioctl(fd3, IOC_SET_DIR, dirs1);
    sleep(1);

    ioctl(fd0, IOC_GET_DIR, &dirs2);
    printf("SET DIR value0=%08X, mask0=%08X\n", dirs1.value, dirs1.mask);
    printf("GET DIR value0=%08X, mask0=%08X\n", dirs2.value, dirs2.mask);
    ioctl(fd1, IOC_GET_DIR, &dirs2);
    printf("SET DIR value1=%08X, mask1=%08X\n", dirs1.value, dirs1.mask);
    printf("GET DIR value1=%08X, mask1=%08X\n", dirs2.value, dirs2.mask);
    ioctl(fd2, IOC_GET_DIR, &dirs2);
    printf("SET DIR value2=%08X, mask2=%08X\n", dirs1.value, dirs1.mask);
    printf("GET DIR value2=%08X, mask2=%08X\n", dirs2.value, dirs2.mask);
    ioctl(fd3, IOC_GET_DIR, &dirs2);
    printf("SET DIR value3=%08X, mask3=%08X\n", dirs1.value, dirs1.mask);
    printf("GET DIR value3=%08X, mask3=%08X\n", dirs2.value, dirs2.mask);
    
    close(fd0);
    close(fd1);
    close(fd2);
    close(fd3);
    return 0;
}

int test_read(const int idx, const char* file_path)
{
    struct et_gpio_data dirs;
    struct et_gpio_data data;
    int i;
    int pos;
    int fd = open(file_path, O_RDONLY);
    
    printf("\n%s test begin\n", file_path);
    sleep(1);

    pos = 0;
    while (pos < CHANNEL_WIDTH) {
        dirs.value = 1 << pos;
        dirs.mask = 1 << pos;
        ioctl(fd, IOC_SET_DIR, dirs);
        i = 0;
        while (i++<30) {
            read(fd, &data, sizeof(struct et_gpio_data));
            printf("card%d value: %08X, mask: %08X\n", idx, data.value, data.mask);
            sleep(1);
        }
        pos++;
    }
    close(fd);
    return 0;
}

int test_write()
{
    int fd0 = open("/dev/et_gpio0", O_RDWR);
    int fd1 = open("/dev/et_gpio1", O_RDWR);
    int fd2 = open("/dev/et_gpio2", O_RDWR);
    int fd3 = open("/dev/et_gpio3", O_RDWR);

    struct et_gpio_data dirs0, dirs1,dirs2,dirs3;
    dirs0.value = 0x0;
    dirs0.mask = FULL_WIDTH;
    dirs1.value = 0x0;
    dirs1.mask = FULL_WIDTH;
    dirs2.value = 0x0;
    dirs2.mask = FULL_WIDTH;
    dirs3.value = 0x0;
    dirs3.mask = FULL_WIDTH;
    ioctl(fd0, IOC_SET_DIR, dirs0);
    ioctl(fd1, IOC_SET_DIR, dirs1);
    ioctl(fd2, IOC_SET_DIR, dirs2);
    ioctl(fd3, IOC_SET_DIR, dirs3);
    sleep(1);

    struct et_gpio_data data;
    int i = 0;
    while (i++<30) {
        data.value = data.value ? 0 : 0xFFFFFFFF;
        data.mask = FULL_WIDTH;
        write(fd1, &data, sizeof(struct et_gpio_data));
        write(fd0, &data, sizeof(struct et_gpio_data));
        write(fd2, &data, sizeof(struct et_gpio_data));
        write(fd3, &data, sizeof(struct et_gpio_data)); 
        printf("all write %08X\n", data.value);
        sleep(1);
    }

    close(fd0);
    close(fd1);
    close(fd2);
    close(fd3);
    return 0;
}

int test_poll_15s(const char* file_path)
{
    struct epoll_event evs;
    int err, epfd, fd;
    struct et_gpio_data dirs ={
        .value = 0xFFFFFFFF,
        .mask = FULL_WIDTH,       
    }, d;

    fd = open(file_path, O_RDWR | O_NONBLOCK);
    if(!fd) {
        printf("ERROR LINE:%d\n", __LINE__);
        err = -1;
        goto CLEAN;
    }

    if(ioctl(fd, IOC_SET_DIR, dirs)) {
        printf("ERROR LINE:%d\n", __LINE__);
        err = -1;
        goto CLEAN;
    }
    sleep(1);

    epfd = epoll_create(1);
    if(epfd<0) {
        printf("ERROR LINE:%d\n", __LINE__);
        err = epfd;
        goto CLEAN;
    }

    evs.events = EPOLLPRI;
    err = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evs);
    if(err<0) {
        printf("ERROR LINE:%d\n", __LINE__);
        goto CLEAN;
    }

    ioctl(fd, IOC_POLL_START, 500); //0.5ms tick

    printf("\nbegin poll on \"%s\"\n", file_path);
    err = ioctl(fd, IOC_POLL_GET, &d);
    err = epoll_wait(epfd, &evs, 1, 15000);

    if(err<0) {
        printf("ERROR LINE:%d\n", __LINE__);
        goto CLEAN;
    } else {
        struct et_gpio_data d;
        ioctl(fd, IOC_POLL_GET, &d);
        printf("poll_result=%d value=%08X mask=%08X\n", err, d.value & d.mask, d.mask);
        err = 0;
        goto CLEAN;
    }

CLEAN:
    if(fd>0) {
        close(fd);
    }
    if(epfd>0) {
        close(epfd);
    }
    return err;
}

int test_poll_loop()
{
    struct epoll_event evs[5];
    int err, epfd;
    int fd0 = open("/dev/et_gpio0", O_RDWR);
    int fd1 = open("/dev/et_gpio1", O_RDWR);
    int fd2 = open("/dev/et_gpio2", O_RDWR);
    int fd3 = open("/dev/et_gpio3", O_RDWR);

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

    epfd = epoll_create(5);
    evs[0].data.fd = fd0;
    evs[0].events = EPOLLPRI;
    evs[1].data.fd = fd1;
    evs[1].events = EPOLLPRI;
    evs[2].data.fd = fd2;
    evs[2].events = EPOLLPRI;
    evs[3].data.fd = fd3;
    evs[3].events = EPOLLPRI;
    err = epoll_ctl(epfd, EPOLL_CTL_ADD, fd0, &evs[0]);
    err = epoll_ctl(epfd, EPOLL_CTL_ADD, fd1, &evs[1]);
    err = epoll_ctl(epfd, EPOLL_CTL_ADD, fd2, &evs[2]);
    err = epoll_ctl(epfd, EPOLL_CTL_ADD, fd3, &evs[3]);
    if(err<0) {
        printf("ERROR LINE:%d\n", __LINE__);
        goto CLEAN;
    }

    struct et_gpio_data d0, d1, d2, d3;

    err = ioctl(fd0, IOC_POLL_START, 500); //0.5ms tick
    err = ioctl(fd1, IOC_POLL_START, 500); //0.5ms tick
    err = ioctl(fd2, IOC_POLL_START, 500); //0.5ms tick
    err = ioctl(fd3, IOC_POLL_START, 500); //0.5ms tick
    if(err<0) {
        printf("ERROR LINE:%d\n", __LINE__);
        goto CLEAN;
    }

    int c = 0;
    printf("begin poll times%d:\n", c);
    while (1)
    {
        err = epoll_wait(epfd, evs, 5, 0);
        if(err<0) {
            printf("ERROR LINE:%d\n", __LINE__);
            goto CLEAN;
        } else if(err>0) {
            for (int i = 0; i < err; i++) {
                struct et_gpio_data d;
                char card[20];
                if(evs[i].data.fd == fd0) {
                    sprintf(card, "%s", "et_gpio_0");
                } else if(evs[i].data.fd == fd1) {
                    sprintf(card, "%s", "et_gpio_1");
                } else if(evs[i].data.fd == fd2) {
                    sprintf(card, "%s", "et_gpio_2");
                } else if(evs[i].data.fd == fd3) {
                    sprintf(card, "%s", "et_gpio_3");
                } else {
                    sprintf(card, "%s", "NULL");
                }
                ioctl(evs[i].data.fd, IOC_POLL_GET, &d);
                printf("change on %s : value=%08X mask=%08X\n", card, d.value & d.mask, d.mask);
            }

            err = ioctl(fd0, IOC_POLL_STOP);
            err = ioctl(fd1, IOC_POLL_STOP);
            err = ioctl(fd2, IOC_POLL_STOP);
            err = ioctl(fd3, IOC_POLL_STOP);
            if(err<0) {
                printf("ERROR LINE:%d\n", __LINE__);
                goto CLEAN;
            }

            sleep(1);
            if(c++>14) {
                goto CLEAN;
            }

            printf("\nbegin poll times%d:\n", c);
            err = ioctl(fd0, IOC_POLL_START, 500); //0.5ms tick
            err = ioctl(fd1, IOC_POLL_START, 500); //0.5ms tick
            err = ioctl(fd2, IOC_POLL_START, 500); //0.5ms tick
            err = ioctl(fd3, IOC_POLL_START, 500); //0.5ms tick
            if(err<0) {
                printf("ERROR LINE:%d\n", __LINE__);
                goto CLEAN;
    }
        }
    }

CLEAN:
    if(fd0>0) {
        close(fd0);
    }
    if(fd1>0) {
        close(fd1);
    }
    if(fd2>0) {
        close(fd2);
    }
    if(fd3>0) {
        close(fd3);
    }
    if(epfd>0) {
        close(epfd);
    }
    return err;
}

int test()
{
    printf("\na:test_read\n");
    printf("b:test_write\n");
    printf("c:test_dir\n");
    printf("d:test_poll_15s\n");
    printf("e:test_poll_loop\n");
    printf("q:quit\n");
    printf("Input char to test---------------\n");

    char c;

INPUT:
    c=getchar();
    
    int err = 0;

    switch (c)
    {
        case 'a':
            printf("-----------test_read---------\n");
            err = test_read(0, "/dev/et_gpio0");
            if(err) 
                break;
            err = test_read(1, "/dev/et_gpio1");
            if(err) 
                break;
            err = test_read(2, "/dev/et_gpio2");
            if(err) 
                break;
            err = test_read(3, "/dev/et_gpio3");
            break;
        case 'b':
            printf("-----------test_write-------------\n");
            err = test_write();
            break;
        case 'c':
            printf("-----------test_dir----------------\n");
            err = test_dir();
            break;
        case 'd':
            printf("-----------test_poll_15s----------------\n");
            err = test_poll_15s("/dev/et_gpio0");
            if(err) 
                break;
            err = test_poll_15s("/dev/et_gpio1");
            if(err) 
                break;
            err = test_poll_15s("/dev/et_gpio2");
            if(err) 
                break;
            err = test_poll_15s("/dev/et_gpio3");
            break;
        case 'e':
            printf("-----------test_poll_loop----------------\n");
            err = test_poll_loop();
            break;
        case 'q':
            err = -1;
            break;

        default:
            goto INPUT;
    }
    return err;
}

int main(int argc, char **argv)
{
    while (1)
    {
        int res = test();
        if(res != 0)
            return 0;
    }
}
