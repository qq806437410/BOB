#include <linux/init.h> //提供初始化和退出函数的宏定义。
#include <linux/module.h> //用于定义模块的基本信息和属性
#include <linux/fs.h> //用于文件系统的操作
#include <linux/cdev.h> //用于字符设备的注册和管理
#include <linux/gpio.h> //
#include <linux/delay.h> //
#include <linux/poll.h> //用于实现文件操作的等待机制（本驱动未使用）
#include <linux/sched.h> //提供用户空间和内核空间数据交互的操作
#include <linux/wait.h> //
#include <asm/uaccess.h> //提供用户空间和内核空间数据交互的操作
#include <linux/device.h> //提供设备模型的相关函数
#include <linux/kobject.h> //用于管理 sysfs 相关的对象

#define COUNT 1
#define LED_IO 199 // GPIO 编号，根据实际情况进行更改
#define dev_name "my_led_driver"

dev_t led_device_id;
struct cdev led_cdev;
static struct class *led_class;
static struct kobject *led_kobj; // sysfs 中的 kobject

static int led_status = 0; // 记录 LED 的状态

// 设置 LED 状态的函数
static ssize_t led_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	if (buf[0] == '1') {
		gpio_set_value(LED_IO, 1);
		led_status = 1;
		printk("LED is ON\n");
	} else if (buf[0] == '0') {
		gpio_set_value(LED_IO, 0);
		led_status = 0;
		printk("LED is OFF\n");
	}
	return count;
}

// 读取 LED 状态的函数
static ssize_t led_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", led_status);
}

// 创建 sysfs 属性
static struct kobj_attribute led_attr = __ATTR(led, 0664, led_show, led_store);

static int led_open(struct inode *inode_, struct file *file_)
{
	printk("my led_driver is successful to open!\n");
	int ret;

	ret = gpio_request(LED_IO, "led_0");
	if (ret < 0) {
		return ret;
	} else {
		gpio_direction_output(LED_IO, 1);
		led_status = 1;
		return 0;
	}
}

static ssize_t led_write(struct file *file_, const char *__user buf_,
			 size_t len_, loff_t *loff_)
{
	char stat;
	int ret;

	ret = copy_from_user(&stat, buf_, sizeof(char));
	if (ret < 0)
		return ret;

	if (stat == '0') {
		gpio_set_value(LED_IO, 0);
		led_status = 0;
	} else if (stat == '1') {
		gpio_set_value(LED_IO, 1);
		led_status = 1;
	}
	return 0;
}

static int led_close(struct inode *inode_, struct file *file_)
{
	gpio_set_value(LED_IO, 0);
	gpio_free(LED_IO);
	return 0;
}

static struct file_operations led_file_operations = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
	.release = led_close,
};
/**
 * @brief   led_init：模块加载时调用的初始化函数。
            MKDEV(502, 502) 创建设备号。
            register_chrdev_region 注册字符设备区域。
            cdev_init 和 cdev_add 初始化并添加字符设备。
            class_create 和 device_create 创建设备类和设备节点。
            创建 sysfs 接口以供用户空间访问。
 * 
 * @return int 
 */
static int __init led_init(void)
{
	int ret;

	// 创建设备节点
	led_device_id = MKDEV(502, 502);
	ret = register_chrdev_region(led_device_id, COUNT, dev_name);
	if (ret < 0) {
		return ret;
	}

	cdev_init(&led_cdev, &led_file_operations);
	led_cdev.owner = THIS_MODULE;
	ret = cdev_add(&led_cdev, led_device_id, COUNT);
	if (ret < 0) {
		unregister_chrdev_region(led_device_id, COUNT);
		return ret;
	}

	led_class = class_create(THIS_MODULE, "led_class");
	if (IS_ERR(led_class)) {
		cdev_del(&led_cdev);
		unregister_chrdev_region(led_device_id, COUNT);
		return PTR_ERR(led_class);
	}

	device_create(led_class, NULL, led_device_id, NULL, dev_name);

	// 创建 sysfs 接口
	led_kobj = kobject_create_and_add("led_kobj", kernel_kobj);
	if (!led_kobj)
		return -ENOMEM;

	ret = sysfs_create_file(led_kobj, &led_attr.attr);
	if (ret) {
		kobject_put(led_kobj);
		return ret;
	}

	return 0;
}

static void __exit led_exit(void)
{
	sysfs_remove_file(led_kobj, &led_attr.attr);
	kobject_put(led_kobj);

	device_destroy(led_class, led_device_id);
	class_destroy(led_class);
	unregister_chrdev_region(led_device_id, COUNT);
	cdev_del(&led_cdev);
	gpio_free(LED_IO);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hucheng");
MODULE_DESCRIPTION("A simple Linux char driver for LEDs with sysfs support");
MODULE_VERSION("0.2");
