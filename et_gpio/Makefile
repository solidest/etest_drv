
CONFIG_MODULE_SIG = n

KVERS = $(shell uname -r)

obj-m += et_gpio_drv.o #et_gpio_plat_drv.o et_gpio_plat_dev.o

build: kernel_modules

kernel_modules:
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
	rm -f *.o *~ core .depend .*.cmd *.mod *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers