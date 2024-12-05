#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/delay.h>

/**
 * @brief 关于取高低位的说明
 *  
        * uint16_t data = 0xABCD;
        uint8_t buffer[2];

        获取高 8 位
        buffer[0] = (data >> 8) & 0xFF;  // 结果：buffer[0] = 0xAB

        获取低 8 位
        buffer[1] = data & 0xFF;         // 结果：buffer[1] = 0xCD
 * 
 */

// 定义 GP8202AS 默认 I2C 地址
#define GP8202AS_DEFAULT_ADDR 0x02

// 向 GP8202AS 写入 12bit 数据
static int gp8202as_write(struct i2c_client *client, uint16_t data)
{
	uint8_t buffer[2];

	buffer[0] = (data >> 8) & 0xFF; // 高 8 位
	buffer[1] = data & 0xFF; // 低 8 位

	return i2c_master_send(client, buffer, 2); // 通过 I2C 发送 2 字节数据
}

// 驱动探测函数
static int gp8202as_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	int ret;
	uint16_t init_data = 0x7FF;
	uint8_t addr;

	dev_info(&client->dev, "Probing GP8202AS...\n");

	// 尝试常见地址范围
	for (addr = 0x00; addr <= 0x7F; addr++) {
		client->addr = addr;
		dev_info(&client->dev, "Testing address 0x%02x\n", addr);

		ret = gp8202as_write(client, init_data);
		if (ret >= 0) {
			dev_info(&client->dev,
				 "GP8202AS found at address 0x%02x\n", addr);
			return 0; // 成功退出
		}
	}

	dev_err(&client->dev, "Failed to find GP8202AS\n");
	return -ENODEV;
}

// 驱动移除函数
static int gp8202as_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "GP8202AS removed\n");
	return 0;
}

// 设备树兼容表
static const struct of_device_id gp8202as_of_match[] = {
	{
		.compatible = "gp8202as",
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, gp8202as_of_match);

// I2C 设备 ID 表
static const struct i2c_device_id gp8202as_id[] = { { "gp8202as", 0 },
						    { /* sentinel */ } };
MODULE_DEVICE_TABLE(i2c, gp8202as_id);

// I2C 驱动结构
static struct i2c_driver gp8202as_driver = {
    .driver = {
        .name = "gp8202as",
        .of_match_table = gp8202as_of_match,
    },
    .probe = gp8202as_probe,
    .remove = gp8202as_remove,
    .id_table = gp8202as_id,
};

// 驱动初始化函数
static int __init gp8202as_init(void)
{
	return i2c_add_driver(&gp8202as_driver);
}

// 驱动卸载函数
static void __exit gp8202as_exit(void)
{
	i2c_del_driver(&gp8202as_driver);
}

module_init(gp8202as_init);
module_exit(gp8202as_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cheng.hu");
MODULE_DESCRIPTION("GP8202AS DAC Driver.2024.11.27");