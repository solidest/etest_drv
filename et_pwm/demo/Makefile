KVERS = $(shell uname -r)

obj-m += et_pwm_drv.o

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers