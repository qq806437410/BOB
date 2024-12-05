# 指定要编译的对象文件
                     
obj-$(CONFIG_DEMO_DRIVER) += demo_driver.o

obj-$(CONFIG_LED_DRIVER) += led_driver.o

obj-$(CONFIG_FC_GPIO_DRIVER) += gpio_driver.o

obj-$(CONFIG_FC_GP8202AS_DRIVER) += GP8202AS.o

# 编译内核模块
all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

# 清理编译生成的文件
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

