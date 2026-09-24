#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>

#define DEMO_I2C_NAME "demo-i2c-sensor"
#define DEMO_SPI_NAME "demo-spi-sensor"

static const struct i2c_device_id demo_i2c_ids[] = {
	{ DEMO_I2C_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, demo_i2c_ids);

static const struct of_device_id demo_i2c_of_match[] = {
	{ .compatible = "demo,i2c-sensor" },
	{ }
};
MODULE_DEVICE_TABLE(of, demo_i2c_of_match);

static int demo_i2c_probe(struct i2c_client *client)
{
	dev_info(&client->dev,
		 "I2C probe: matched device at addr=0x%02x\n",
		 client->addr);
	return 0;
}

static void demo_i2c_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "I2C remove: device released\n");
}

static struct i2c_driver demo_i2c_driver = {
	.driver = {
		.name = DEMO_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = demo_i2c_of_match,
	},
	.probe = demo_i2c_probe,
	.remove = demo_i2c_remove,
	.id_table = demo_i2c_ids,
};

static const struct of_device_id demo_spi_of_match[] = {
	{ .compatible = "demo,spi-sensor" },
	{ }
};
MODULE_DEVICE_TABLE(of, demo_spi_of_match);

static const struct spi_device_id demo_spi_ids[] = {
	{ DEMO_SPI_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, demo_spi_ids);

static int demo_spi_probe(struct spi_device *spi)
{
	dev_info(&spi->dev,
		 "SPI probe: matched device '%s', mode=%d, max_speed=%u Hz\n",
		 spi->modalias, spi->mode, spi->max_speed_hz);
	return 0;
}

static void demo_spi_remove(struct spi_device *spi)
{
	dev_info(&spi->dev, "SPI remove: device released\n");
}

static struct spi_driver demo_spi_driver = {
	.driver = {
		.name = DEMO_SPI_NAME,
		.owner = THIS_MODULE,
		.of_match_table = demo_spi_of_match,
	},
	.probe = demo_spi_probe,
	.remove = demo_spi_remove,
	.id_table = demo_spi_ids,
};

static int __init demo_i2c_spi_init(void)
{
	int ret;

	ret = i2c_add_driver(&demo_i2c_driver);
	if (ret) {
		pr_err("demo_i2c_spi: I2C-Treiber konnte nicht registriert werden\n");
		return ret;
	}

	ret = spi_register_driver(&demo_spi_driver);
	if (ret) {
		pr_err("demo_i2c_spi: SPI-Treiber konnte nicht registriert werden\n");
		i2c_del_driver(&demo_i2c_driver);
		return ret;
	}

	pr_info("demo_i2c_spi: I2C/SPI-Treiber registriert\n");
	return 0;
}

static void __exit demo_i2c_spi_exit(void)
{
	spi_unregister_driver(&demo_spi_driver);
	i2c_del_driver(&demo_i2c_driver);
	pr_info("demo_i2c_spi: I2C/SPI-Treiber entfernt\n");
}

module_init(demo_i2c_spi_init);
module_exit(demo_i2c_spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Internals Training");
MODULE_DESCRIPTION("Trainingsmodul fuer I2C- und SPI-Treiber-Matching");
