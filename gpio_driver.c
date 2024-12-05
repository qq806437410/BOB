#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "gpio_device"
#define CLASS_NAME "gpio_class"

// 定义GPIO引脚
#define GPIO_PIN_IN1 130
#define GPIO_PIN_IN2 135
#define GPIO_PIN_IN3 134
#define GPIO_PIN_IN4 133
#define GPIO_PIN_IN5 132
#define GPIO_PIN_IN6 128
#define GPIO_PIN_IN7 129

#define GPIO_PIN_OUT1 160
#define GPIO_PIN_OUT2 161
#define GPIO_PIN_OUT3 162
#define GPIO_PIN_OUT4 163
#define GPIO_PIN_OUT5 164
#define GPIO_PIN_OUT6 165
#define GPIO_PIN_OUT7 166

// ioctl命令
#define GPIO_IOCTL_BASE 'G'
#define GPIO_SET_PIN                                                           \
	_IOW(GPIO_IOCTL_BASE, 1, struct gpio_ioctl_args) // 设置输出引脚
#define GPIO_GET_PIN                                                           \
	_IOR(GPIO_IOCTL_BASE, 2, struct gpio_ioctl_args) // 读取输入引脚

static int major_number;
static struct class *gpio_class = NULL;
static struct device *gpio_device = NULL;

struct gpio_ioctl_args {
	int pin; // GPIO引脚
	int value; // 引脚值（0 或 1）
};

// 初始化GPIO引脚
static int gpio_init_pins(void)
{
	int ret, i;

	// 请求输入引脚
	int inputs[] = { GPIO_PIN_IN1, GPIO_PIN_IN2, GPIO_PIN_IN3, GPIO_PIN_IN4,
			 GPIO_PIN_IN5, GPIO_PIN_IN6, GPIO_PIN_IN7 };
	for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
		if (!gpio_is_valid(inputs[i])) {
			printk(KERN_ERR "GPIO_IN%d is not valid\n", i + 1);
			return -EINVAL; // 返回无效错误
		}

		ret = gpio_request(inputs[i], "gpio_input");
		if (ret) {
			printk(KERN_ERR "Failed to request GPIO_IN%d: %d\n",
			       i + 1, ret); // 打印错误码
			return ret;
		}
		gpio_direction_input(inputs[i]);
		printk(KERN_INFO "Successfully requested GPIO_IN%d\n",
		       i + 1); // 打印成功信息
	}

	// 请求输出引脚
	int outputs[] = { GPIO_PIN_OUT1, GPIO_PIN_OUT2, GPIO_PIN_OUT3,
			  GPIO_PIN_OUT4, GPIO_PIN_OUT5, GPIO_PIN_OUT6,
			  GPIO_PIN_OUT7 };
	for (i = 0; i < sizeof(outputs) / sizeof(outputs[0]); i++) {
		ret = gpio_request(outputs[i], "gpio_output");
		if (ret) {
			printk(KERN_ERR "Failed to request GPIO_OUT%d: %d\n",
			       i + 1, ret); // 打印错误码
			return ret;
		}
		gpio_direction_output(outputs[i], 0); // 初始为低电平
		printk(KERN_INFO "Successfully requested GPIO_OUT%d\n",
		       i + 1); // 打印成功信息
	}
	return 0;
}

// 释放GPIO引脚
static void gpio_free_pins(void)
{
	int i;
	int inputs[] = { GPIO_PIN_IN1, GPIO_PIN_IN2, GPIO_PIN_IN3, GPIO_PIN_IN4,
			 GPIO_PIN_IN5, GPIO_PIN_IN6, GPIO_PIN_IN7 };
	int outputs[] = { GPIO_PIN_OUT1, GPIO_PIN_OUT2, GPIO_PIN_OUT3,
			  GPIO_PIN_OUT4, GPIO_PIN_OUT5, GPIO_PIN_OUT6,
			  GPIO_PIN_OUT7 };

	for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
		gpio_free(inputs[i]);
	}
	for (i = 0; i < sizeof(outputs) / sizeof(outputs[0]); i++) {
		gpio_free(outputs[i]);
	}
}

// 设备文件操作 - IOCTL函数
// 修改 gpio_ioctl 函数，支持传递电平值
static long gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct gpio_ioctl_args args;
	int pin, value;

	switch (cmd) {
	case GPIO_SET_PIN:
		if (copy_from_user(&args, (struct gpio_ioctl_args *)arg,
				   sizeof(args))) {
			return -EFAULT;
		}
		pin = args.pin;
		value = args.value;

		if (pin >= GPIO_PIN_OUT1 && pin <= GPIO_PIN_OUT7) {
			gpio_set_value(pin, value); // 设置为传入的电平值
			// printk(KERN_INFO "Set GPIO %d to %d\n", pin, value);
		} else {
			printk(KERN_ERR "Invalid GPIO pin for output: %d\n",
			       pin);
			return -EINVAL;
		}
		break;

	case GPIO_GET_PIN:
		if (copy_from_user(&args.pin, (int *)arg, sizeof(int))) {
			return -EFAULT;
		}
		pin = args.pin;

		if (pin >= GPIO_PIN_IN6 && pin <= GPIO_PIN_IN2) {
			value = gpio_get_value(pin);
			args.value = value;
			if (copy_to_user((struct gpio_ioctl_args *)arg, &args,
					 sizeof(args))) {
				return -EFAULT;
			}
			// printk(KERN_INFO "Read GPIO %d: %d\n", pin, value);
		} else {
			printk(KERN_ERR "Invalid GPIO pin for input: %d\n",
			       pin);
			return -EINVAL;
		}
		break;

	default:
		return -ENOTTY;
	}
	return 0;
}

// 文件操作接口
static struct file_operations fops = {
	.unlocked_ioctl = gpio_ioctl,
};

// 初始化模块
static int __init gpio_driver_init(void)
{
	int ret;

	// 初始化GPIO引脚
	ret = gpio_init_pins();
	if (ret) {
		printk(KERN_ERR "Failed to initialize GPIO pins\n");
		return ret;
	}

	// 注册字符设备
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if (major_number < 0) {
		printk(KERN_ERR "Failed to register a major number\n");
		gpio_free_pins();
		return major_number;
	}

	// 创建设备类
	gpio_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(gpio_class)) {
		unregister_chrdev(major_number, DEVICE_NAME);
		gpio_free_pins();
		return PTR_ERR(gpio_class);
	}

	// 创建设备节点
	gpio_device = device_create(gpio_class, NULL, MKDEV(major_number, 0),
				    NULL, DEVICE_NAME);
	if (IS_ERR(gpio_device)) {
		class_destroy(gpio_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		gpio_free_pins();
		return PTR_ERR(gpio_device);
	}

	printk(KERN_INFO "GPIO driver initialized successfully\n");
	return 0;
}

// 退出模块
static void __exit gpio_driver_exit(void)
{
	device_destroy(gpio_class, MKDEV(major_number, 0));
	class_unregister(gpio_class);
	class_destroy(gpio_class);
	unregister_chrdev(major_number, DEVICE_NAME);

	gpio_free_pins();
	printk(KERN_INFO "GPIO driver exited\n");
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hu cheng");
MODULE_DESCRIPTION(
	"A GPIO control driver with custom IOCTL interface for flow-controller");
