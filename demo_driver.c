#include <linux/module.h> // 包含模块宏定义
#include <linux/kernel.h> // 包含内核信息相关的函数
#include <linux/fs.h> // 包含文件操作结构体
#include <linux/uaccess.h> // 包含用户空间与内核空间的数据复制函数
#include <linux/cdev.h> // 包含字符设备相关的结构体和宏
#include <linux/device.h> // 包含设备类相关的函数
#include <linux/slab.h> // 包含内存分配函数

#define DEVICE_NAME "mychardev" // 设备名称
#define CLASS_NAME "myclass" // 设备类名称

MODULE_LICENSE("GPL"); // 模块许可证
MODULE_AUTHOR("Your Name"); // 模块作者
MODULE_DESCRIPTION("A simple Linux char driver"); // 模块描述
MODULE_VERSION("1.0"); // 模块版本

static int majorNumber; // 主设备号
static struct class *mycharClass = NULL; // 设备类指针
static struct device *mycharDevice = NULL; // 设备指针
static char message[256] = { 0 }; // 存储来自用户空间的数据
static short size_of_message; // 数据的大小

// 设备打开函数
static int dev_open(struct inode *inodep, struct file *filep)
{
	printk(KERN_INFO "mychardev: Device opened\n");
	return 0; // 返回0表示成功
}

// 设备关闭函数
static int dev_release(struct inode *inodep, struct file *filep)
{
	printk(KERN_INFO "mychardev: Device closed\n");
	return 0; // 返回0表示成功
}

// 设备读取函数
static ssize_t dev_read(struct file *filep, char *buffer, size_t len,
			loff_t *offset)
{
	int error_count = 0;
	// 将内核空间的数据复制到用户空间
	error_count = copy_to_user(buffer, message, size_of_message);

	if (error_count == 0) {
		printk(KERN_INFO "mychardev: Sent %d characters to the user\n",
		       size_of_message);
		return (size_of_message = 0); // 清空数据，返回0表示EOF
	} else {
		printk(KERN_INFO
		       "mychardev: Failed to send %d characters to the user\n",
		       error_count);
		return -EFAULT; // 返回错误代码
	}
}

// 设备写入函数
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len,
			 loff_t *offset)
{
	snprintf(message, sizeof(message), "%s(%zu letters)", buffer,
		 len); // 将用户输入的消息复制到内核空间
	size_of_message = strlen(message); // 记录消息长度
	printk(KERN_INFO "mychardev: Received %zu characters from the user\n",
	       len);
	return len; // 返回写入的字节数
}

// 文件操作结构体，关联上述函数
static struct file_operations fops = {
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
	.write = dev_write,
};

// 驱动初始化函数
static int __init mychardev_init(void)
{
	printk(KERN_INFO "mychardev: Initializing the mychardev\n");

	// 动态分配主设备号
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber < 0) {
		printk(KERN_ALERT
		       "mychardev: Failed to register a major number\n");
		return majorNumber; // 返回错误代码
	}
	printk(KERN_INFO
	       "mychardev: Registered correctly with major number %d\n",
	       majorNumber);

	// 注册设备类
	mycharClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(mycharClass)) {
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT
		       "mychardev: Failed to register device class\n");
		return PTR_ERR(mycharClass); // 返回错误代码
	}
	printk(KERN_INFO "mychardev: Device class registered correctly\n");

	// 创建设备节点
	mycharDevice = device_create(mycharClass, NULL, MKDEV(majorNumber, 0),
				     NULL, DEVICE_NAME);
	if (IS_ERR(mycharDevice)) {
		class_destroy(mycharClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "mychardev: Failed to create the device\n");
		return PTR_ERR(mycharDevice); // 返回错误代码
	}
	printk(KERN_INFO "mychardev: Device class created correctly\n");
	return 0; // 返回0表示初始化成功
}

// 驱动退出函数
static void __exit mychardev_exit(void)
{
	device_destroy(mycharClass, MKDEV(majorNumber, 0)); // 删除设备
	class_unregister(mycharClass); // 注销设备类
	class_destroy(mycharClass); // 销毁设备类
	unregister_chrdev(majorNumber, DEVICE_NAME); // 注销设备号
	printk(KERN_INFO "mychardev: Goodbye from the LKM!\n");
}

// 注册初始化和退出函数
module_init(mychardev_init);
module_exit(mychardev_exit);
