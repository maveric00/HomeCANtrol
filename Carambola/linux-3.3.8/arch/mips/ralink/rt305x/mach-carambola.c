/*
 *  CARAMBOLA board support
 *
 *  Copyright (C) 2011 Darius Augulis <darius@8devices.com>
 *  Copyright (C) 2012 Žilvinas Valinskas <zilvinas@8devices.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/spi/spi.h>
#include <linux/can.h>
#include <linux/can/platform/mcp251x.h>

#include <asm/mach-ralink/machine.h>
#include <asm/mach-ralink/rt305x.h>
#include <asm/mach-ralink/rt305x_regs.h>
#include <asm/sizes.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>

#include "devices.h"

#define CARAMBOLA_UBOOT_SIZE	0x030000 /*  192KB */
#define CARAMBOLA_UBOOT_ENV	0x010000 /*   64KB */
#define CARAMBOLA_FACTORY_SIZE	0x010000 /*   64KB */
#define CARAMBOLA_KERNEL_SIZE	0x120000 /* 1152KB */
#define CARAMBOLA_ROOTFS_SIZE	0x690000 /* 6720KB */

static struct mtd_partition carambola_partitions[] = {
	{
		.name   = "u-boot",
		.offset = 0,
		.size   = CARAMBOLA_UBOOT_SIZE,
		.mask_flags = MTD_WRITEABLE,	/* precaution */
	}, {
		.name   = "u-boot-env",
		.offset = MTDPART_OFS_APPEND,
		.size   = CARAMBOLA_UBOOT_ENV,
	}, {
		.name   = "factory",
		.offset = MTDPART_OFS_APPEND,
		.size   = CARAMBOLA_FACTORY_SIZE,
	}, {
		.name   = "kernel",
		.offset = MTDPART_OFS_APPEND,
		.size   = CARAMBOLA_KERNEL_SIZE,
	}, {
		.name   = "rootfs",
		.offset = MTDPART_OFS_APPEND,
		.size	= CARAMBOLA_ROOTFS_SIZE,
	}, {
		.name   = "firmware", /* skip u-boot, uboot-env and factory partitions */
		.offset = CARAMBOLA_UBOOT_SIZE + CARAMBOLA_UBOOT_ENV + CARAMBOLA_FACTORY_SIZE,
		.size   = CARAMBOLA_KERNEL_SIZE + CARAMBOLA_ROOTFS_SIZE,
	}
};


static int __init carambola_register_gpiodev(void)
{
       static struct resource res = {
               .start = 0xFFFFFFFF,
       };
       struct platform_device *pdev;

       pdev = platform_device_register_simple("GPIODEV", 0, &res, 1);
       if (!pdev) {
               printk(KERN_ERR "carambole: GPIODEV init failed\n");
               return -ENODEV;
       }

       return 0;
}

static const struct mcp251x_platform_data mcp251x_info1 = {
        .oscillator_frequency   = 16000000,
        .board_specific_setup   = NULL,
        .irq_flags              = 0,
        .power_enable           = NULL,
        .transceiver_enable     = NULL,
};

static const struct mcp251x_platform_data mcp251x_info2 = {
        .oscillator_frequency   = 16000000,
        .board_specific_setup   = NULL,
        .irq_flags              = 0,
        .power_enable           = NULL,
        .transceiver_enable     = NULL,
};


static struct i2c_gpio_platform_data carambola_i2c_gpio_data = {
	.sda_pin        = 1,
	.scl_pin        = 2,
};

static struct platform_device carambola_i2c_gpio = {
	.name           = "i2c-gpio",
	.id             = 0,
	.dev     = {
		.platform_data  = &carambola_i2c_gpio_data,
	},
};

static struct platform_device *carambola_devices[] __initdata = {
        &carambola_i2c_gpio
};

static struct spi_board_info __initdata carambola_spi_info[] = {
	{
	  .modalias	= "mcp2515",
	  .platform_data  = &mcp251x_info1,
	  //	.irq		= &gpio_to_irq(14),
	  .irq		= 53,
	  .max_speed_hz	= 10000000,
	  .mode		= SPI_MODE_0,
	  .bus_num	= 0,
	  .chip_select	= 0,
	},
	{
	  .modalias	= "mcp2515",
	  .platform_data  = &mcp251x_info2,
	  //	.irq		= &gpio_to_irq(14),
	  .irq		= 52,
	  .max_speed_hz	= 10000000,
	  .mode		= SPI_MODE_0,
	  .bus_num	= 0,
	  .chip_select	= 1,
	}
};

static void __init carambola_init(void)
{
	rt305x_gpio_init((RT305X_GPIO_MODE_GPIO_UARTF << RT305X_GPIO_MODE_UART0_SHIFT) |
			 RT305X_GPIO_MODE_I2C);

	carambola_register_gpiodev();
	platform_add_devices(carambola_devices, ARRAY_SIZE(carambola_devices));

	__rt305x_register_flash(0, carambola_partitions, ARRAY_SIZE(carambola_partitions));
	rt305x_register_spi(carambola_spi_info, ARRAY_SIZE(carambola_spi_info));

	rt305x_esw_data.vlan_config = RT305X_ESW_VLAN_CONFIG_WLLLL;
	rt305x_register_ethernet();
	rt305x_register_wifi();
	rt305x_register_wdt();
	rt305x_register_usb();
}

MIPS_MACHINE(RAMIPS_MACH_CARAMBOLA, "CARAMBOLA", "CARAMBOLA",
	     carambola_init);
