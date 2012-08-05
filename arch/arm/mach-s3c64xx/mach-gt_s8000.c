/* linux/arch/arm/mach-s3c64xx/mach-gt_s8000.c
 *
 * Copyright (C) 2011 Dopi <dopi711 at googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Based on mach_gt_i5700.c by
 *  Tomasz Figa <tomasz.figa at gmail.com>
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/leds.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/mfd/max8906.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/android_pmem.h>
#include <linux/reboot.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/f_accessory.h>
#include <linux/usb/android_composite.h>
#include <linux/switch.h>
#include <linux/fsa9480.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/gpio_keys.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/onenand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/bml.h>
#include <linux/power/jet_battery.h>
#include <linux/input/qt5480_ts.h>
#include <linux/wlan_plat.h>
#include <linux/akm8973.h>
#include <linux/spica_bt.h>
#include <linux/sec_jack.h>
#include <linux/vibetonz.h>
#include <linux/leds-regulator.h>
#include <linux/memory_alloc.h>
#include <linux/memblock.h>
#include <linux/i2c/max8906.h>

#include <sound/gt_i5700.h>
#include <sound/max8906-amp.h>

#include <video/s6d05a.h>
#include <video/ams310fn07.h>

#include <media/s5k4ca_platform.h>
#include <media/s3c_fimc.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/regs-fb.h>
#include <mach/map.h>
#include <mach/gpio-cfg.h>
#include <mach/s3c6410.h>
#include <mach/pd.h>
#include <mach/regs-gpio.h>
#include <mach/regs-gpio-memport.h>
#include <mach/regs-sys.h>
#include <mach/regs-syscon-power.h>
#include <mach/regs-clock.h>

#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/setup.h>

#include <plat/regs-sdhci.h>
#include <plat/regs-serial.h>
#include <plat/iic.h>
#include <plat/fb.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/sdhci.h>
#include <plat/keypad.h>
#include <plat/pm.h>
#include <plat/audio.h>

/*
 * GPIOs
 */

/* Outputs */
#define GPIO_BT_RST_N		S3C64XX_GPC(1)	
#define GPIO_BT_WLAN_REG_ON	S3C64XX_GPC(2)	// WLAN_BT_SHUTDOWN
#define GPIO_WLAN_RST_N		S3C64XX_GPC(3)	
#define GPIO_WLAN_WAKE		S3C64XX_GPE(1)	
#define GPIO_MCAM_RST_N		S3C64XX_GPF(3)	
#define GPIO_TFLASH_EN		S3C64XX_GPG(6)	
#define GPIO_CAM_EN		S3C64XX_GPK(1)	
#define GPIO_EARPATH_SEL	S3C64XX_GPK(2)	
#define GPIO_EAR_CP_CODEC_SW	S3C64XX_GPK(3)	
#define GPIO_MAIN_CP_CODEC_SW	S3C64XX_GPK(4)	
#define GPIO_FM_RST		S3C64XX_GPK(5)	
#define GPIO_CAM_VGA_RST_N	S3C64XX_GPK(12)	
#define GPIO_CAM_VGA_STBY_N	S3C64XX_GPK(13)	
#define GPIO_MSENSE_RST		S3C64XX_GPK(14)	
#define GPIO_USIM_BOOT		S3C64XX_GPK(15)	
#define GPIO_5M_EN		S3C64XX_GPL(3)	// SPICA name: CAM_3M_STBY_N
#define GPIO_VIB_EN		S3C64XX_GPL(4)	// Jet name: VIBTONE_EN
#define GPIO_USB_SEL		S3C64XX_GPL(5)	
#define GPIO_TV_EN		S3C64XX_GPL(6)	
#define GPIO_TOUCH_EN		S3C64XX_GPL(7)	
#define GPIO_EXTWKUP		S3C64XX_GPL(11)	
#define GPIO_BT_WAKE		S3C64XX_GPM(4)	
#define GPIO_ALPS_ON		S3C64XX_GPM(5)	
#define GPIO_MAX8906_AMP_EN	S3C64XX_GPN(1)	
#define GPIO_PDA_ACTIVE		S3C64XX_GPN(3)	
#define GPIO_FM_LDO_ON		S3C64XX_GPN(4)	
#define GPIO_PHONE_ON		S3C64XX_GPN(6)	
#define GPIO_CP_BOOT_SEL	S3C64XX_GPO(2)	
#define GPIO_PCM_SEL		S3C64XX_GPO(5)	
#define GPIO_LCD_CS_N		S3C64XX_GPO(6)	
#define GPIO_LCD_SDI		S3C64XX_GPO(7)	
#define GPIO_LCD_RST_N		S3C64XX_GPO(10)	
#define GPIO_LCD_SCLK		S3C64XX_GPO(12)	
#define GPIO_PDA_PS_HOLD	S3C64XX_GPP(13)	
#define GPIO_UART_SEL		S3C64XX_GPQ(2)	
#define GPIO_VT_CP_SW		S3C64XX_GPQ(4)	

/* Inputs */
#define GPIO_PS_VOUT_30		S3C64XX_GPL(12) 
#define GPIO_BOOT_MODE		S3C64XX_GPL(13) //SPICA name: BOOT
#define GPIO_LCD_ID		S3C64XX_GPQ(5)	
#define GPIO_REV1		S3C64XX_GPO(14)	
#define GPIO_REV2		S3C64XX_GPO(15)	
#define GPIO_REV3		S3C64XX_GPO(8)	

/* I2C (externally pulled up) */
#define GPIO_USBSW_SCL_3V0	S3C64XX_GPK(6)	
#define GPIO_USBSW_SDA_3V0	S3C64XX_GPK(7)	
#define GPIO_FM_I2C_SCL		S3C64XX_GPM(0)	
#define GPIO_FM_I2C_SDA		S3C64XX_GPM(1)	
#define GPIO_PWR_I2C_SCL	S3C64XX_GPN(9)	
#define GPIO_PWR_I2C_SDA	S3C64XX_GPN(10)	
#define	GPIO_AP_SCL_1_8V	S3C64XX_GPO(8)	
#define	GPIO_AP_SDA_1_8V	S3C64XX_GPO(9)	

/* EINTs */
#define GPIO_TA_SEL		S3C64XX_GPK(0)	// SPICA name: TA_CONNECTED_N
#define GPIO_TF_DETECT		S3C64XX_GPL(8)	// Jet name: T_FLASH_DETECT
#define GPIO_DET_35		S3C64XX_GPL(9)	
#define GPIO_FM_INT		S3C64XX_GPL(10) 
#define GPIO_ACC_INT		S3C64XX_GPL(14)	
#define GPIO_WLAN_HOST_WAKE	S3C64XX_GPM(2)	
#define GPIO_BT_HOST_WAKE	S3C64XX_GPM(3)	
#define GPIO_ONEDRAM_INT_N	S3C64XX_GPN(0)	
#define GPIO_MSENSE_INT		S3C64XX_GPN(2)	
#define GPIO_POWER_N		S3C64XX_GPN(5)	
#define GPIO_PHONE_ACTIVE	S3C64XX_GPN(7)	
#define GPIO_PMIC_INT_N		S3C64XX_GPN(8)	
#define GPIO_EAR_SEND_END	S3C64XX_GPN(11)	
#define GPIO_JACK_INT_N		S3C64XX_GPN(12)	
#define GPIO_BOOT_EINT13	S3C64XX_GPN(13)	
#define GPIO_BOOT_EINT14	S3C64XX_GPN(14)	
#define GPIO_BOOT_EINT15	S3C64XX_GPN(15)	
#define GPIO_RESOUT_N		S3C64XX_GPO(3)	

/* Other functions */
#define GPIO_KEYSCAN_0		S3C64XX_GPL(0)
#define GPIO_KEYSCAN_1		S3C64XX_GPL(1)
#define GPIO_KEYSCAN_2		S3C64XX_GPL(2)
#define GPIO_MEM0_NCS2		S3C64XX_GPO(0)	

/*
 * External interrupts
 */

/* Low level */
#define IRQ_ONEDRAM		IRQ_EINT(0)	//J
//#define IRQ_WLAN		IRQ_EINT(1)
/* Rising edge */
#define IRQ_MSENSE		IRQ_EINT(2)
//#define IRQ_BMA023		IRQ_EINT(3)
/* Both edges */
//#define IRQ_SIM_DETECT		IRQ_EINT(4)
#define IRQ_POWER		IRQ_EINT(5)	//J
/* Both edges */
//#define IRQ_TF_DETECT		IRQ_EINT(6)
#define IRQ_PHONE_ACTIVE	IRQ_EINT(7)	//J
/* Falling edge */
#define IRQ_PMIC		IRQ_EINT(8)	//J
/* Both edges */
//#define IRQ_JACK		IRQ_EINT(10)
#define IRQ_HEADSET		IRQ_EINT(11)	//J
/* Falling edge */
#define IRQ_FSA9480		IRQ_EINT(12)	//J
/* Falling edge */
#define IRQ_FM			IRQ_EINT(18)
/* Both edges */
#define IRQ_HOLD_KEY		IRQ_EINT(17)
/* Both edges */
#define IRQ_TA_ONLINE		IRQ_EINT(19)
/* Low level */
//#define IRQ_QT5480		IRQ_EINT(20)
/* Both edges */
#define IRQ_BLUETOOTH		IRQ_EINT(22)
/* Both edges */
#define IRQ_WLAN		IRQ_EINT(25)

/*
 * UART
 */

#define UCON S3C2410_UCON_DEFAULT
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

static void spica_bt_uart_wake_peer(struct uart_port *port);

static struct s3c24xx_uart_clksrc spica_uart_clksrcs[] = {
	{
		.name		= "uclk1",
		.min_baud	= 0,
		.max_baud	= 0,
		.divisor	= 1,
	},
};

static struct s3c2410_uartcfg spica_uartcfgs[] __initdata = {
	[0] = {	/* Phone */
		.hwport		= 3,
		.flags		= 0,
		.ucon		= UCON,
		.ulcon		= ULCON,
		.ufcon		= UFCON,
		.clocks		= spica_uart_clksrcs,
		.clocks_size	= ARRAY_SIZE(spica_uart_clksrcs),
	},
	[1] = {	/* Bluetooth */
		.hwport		= 1,
		.flags		= 0,
		.ucon		= UCON,
		.ulcon		= ULCON,
		.ufcon		= UFCON,
		.clocks		= spica_uart_clksrcs,
		.clocks_size	= ARRAY_SIZE(spica_uart_clksrcs),
		.wake_peer	= spica_bt_uart_wake_peer,
	},
	[2] = {	/* Serial */
		.hwport		= 2,
		.flags		= 0,
		.ucon		= UCON,
		.ulcon		= ULCON,
		.ufcon		= UFCON,
		.clocks		= spica_uart_clksrcs,
		.clocks_size	= ARRAY_SIZE(spica_uart_clksrcs),
	},
};

/*
 * I2C devices
 */

/* I2C 0 (hardware) - Sensors, MCam, VGACam
 *	BMA023 (accelerometer),
 * 	AK8973B (magnetometer) 
 */
static struct akm8973_platform_data spica_akm8973_pdata = {
	.gpio_RST = GPIO_MSENSE_RST,
};

static struct i2c_board_info jet_sensor_i2c_devs[] __initdata = {
	{
		.type		= "kionix-kxsd9",
		.addr		= 0x18,
//		.irq		= IRQ_MSENSE,	//IRQ_BMA023,
	}, {
		.type		= "akm8973",
		.addr		= 0x1c,
//		.irq		= IRQ_AKM8973,
		.platform_data	= &spica_akm8973_pdata,
	}
};
static struct s3c2410_platform_i2c jet_sensor_i2c __initdata = {
	.flags		= 0,
	.slave_addr	= 0x10,
	.frequency	= 100*1000,
	.sda_delay	= 100,
	.bus_num	= 0,
};

/* I2C 2 (GPIO) - uUSB switch (USBSW)
 *	FSA9480	
 */
static struct i2c_gpio_platform_data jet_usbsw_i2c_pdata = {
	.sda_pin		= GPIO_USBSW_SDA_3V0,
	.scl_pin		= GPIO_USBSW_SCL_3V0,
	.udelay			= 5, // 2,	/* 250kHz */ 	//3,	/* 166KHz */		
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1,
};

static struct platform_device jet_usbsw_i2c = {
	.name			= "i2c-gpio",
	.id			= 4,
	.dev.platform_data	= &jet_usbsw_i2c_pdata,
};

static bool jet_usb_connected = 0;
static bool jet_ac_connected = 0;

static jet_battery_notify_func_t *jet_battery_notify_func = 0;

static struct platform_device jet_battery;
static void jet_battery_notify(void)
{
	enum jet_battery_supply supply = JET_BATTERY_NONE;

	if (!jet_battery_notify_func)
		return;

	if (jet_usb_connected)
		supply = JET_BATTERY_USB;
	if (jet_ac_connected)
		supply = JET_BATTERY_AC;

	jet_battery_notify_func(&jet_battery, supply);
}

static void jet_usb_callback(bool attached)
{
	jet_usb_connected = attached;
	jet_battery_notify();
}

static void jet_ac_callback(bool attached)
{
	jet_ac_connected = attached;
	jet_battery_notify();
}

static struct fsa9480_platform_data jet_fsa9480_pdata = {
	.usb_cb		= jet_usb_callback,
	.charger_cb	= jet_ac_callback,
};

static struct i2c_board_info jet_usbsw_i2c_devs[] __initdata = {
	{
		.type		= "fsa9480",
		.addr		= 0x25,
		.irq		= IRQ_FSA9480,
		.platform_data	= &jet_fsa9480_pdata,
	}
};


/* I2C 3 (GPIO) - FM, CamLDO
 *	MAX9877AERP-T (audio amplifier),
 *	AK4671EG-L (audio codec) 
 */
static struct i2c_gpio_platform_data jet_fm_i2c_pdata = {
	.sda_pin		= GPIO_FM_I2C_SDA,
	.scl_pin		= GPIO_FM_I2C_SCL,
	.udelay			= 2, /* 250KHz */
};

static struct platform_device jet_fm_i2c = {
	.name			= "i2c-gpio",
	.id			= 2,
	.dev.platform_data	= &jet_fm_i2c_pdata,
};

#if 0
static struct si470x_platform_data jet_si470x_pdata = {
	.gpio_fm_on = GPIO_FM_LDO_ON,
	.gpio_fm_reset = GPIO_FM_RST,
};
#endif

static struct i2c_board_info jet_fm_i2c_devs[] __initdata = {
	{
		.type		= "si470x",
		.addr		= 0x10,
		.irq		= IRQ_FM,
//		.platform_data	= &jet_si470x_pdata,
	}
};

/* I2C 4 (GPIO) - PMIC 
 *	MAX8906 (Power Management IC / voltage regulator) 
 */
static struct i2c_gpio_platform_data jet_pmic_i2c_pdata = {
	.sda_pin		= GPIO_PWR_I2C_SDA,
	.scl_pin		= GPIO_PWR_I2C_SCL,
	.udelay			= 5, // 2,	/* 250KHz */		
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1,
};

static struct platform_device jet_pmic_i2c = {
	.name			= "i2c-gpio",
	.id			= 3,
	.dev.platform_data	= &jet_pmic_i2c_pdata,
};


// LDOA: 150mA 0.75~3.9V (VLCD_3.1V)
static struct regulator_consumer_supply ldoa_consumer[] = {
	REGULATOR_SUPPLY("vdd3", "s6d05a-lcd")
};

static struct regulator_init_data jet_ldoa_data = {
	.constraints	= {
		.name			= "VLCD_3.1V",
		.min_uV			= 3100000,
		.max_uV			= 3100000,
		.apply_uV		= 0,
		.always_on		= 0,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldoa_consumer),
	.consumer_supplies	= ldoa_consumer,
};

/* SPICA definitions
static struct regulator_init_data spica_ldo2_data = {
	.constraints	= {
		.name			= "VAP_ALIVE_1.2V",
		.min_uV			= 1200000,
		.max_uV			= 1200000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.enabled = 1,
		},
	},
};

static struct regulator_consumer_supply ldo3_consumer[] = {
	REGULATOR_SUPPLY("otg-io", "s3c-hsotg")
};

static struct regulator_init_data spica_ldo3_data = {
	.constraints	= {
		.name			= "VAP_OTGI_1.2V",
		.min_uV			= 1200000,
		.max_uV			= 1200000,
		.apply_uV		= 0,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo3_consumer),
	.consumer_supplies	= ldo3_consumer,
};

static struct regulator_init_data spica_ldo4_data = {
	.constraints	= {
		.name			= "VLED_3.3V",
		.min_uV			= 3300000,
		.max_uV			= 3300000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
};

static struct regulator_consumer_supply ldo5_consumer[] = {
	REGULATOR_SUPPLY("vmmc", "s3c-sdhci.0")
};

static struct regulator_init_data spica_ldo5_data = {
	.constraints	= {
		.name			= "VTF_3.0V",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= 0,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo5_consumer),
	.consumer_supplies	= ldo5_consumer,
};

static struct regulator_consumer_supply ldo6_consumer[] = {
	REGULATOR_SUPPLY("vdd3", "s6d05a-lcd")
};

static struct regulator_init_data spica_ldo6_data = {
	.constraints	= {
		.name			= "VLCD_1.8V",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= 0,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo6_consumer),
	.consumer_supplies	= ldo6_consumer,
};

static struct regulator_consumer_supply ldo7_consumer[] = {
	REGULATOR_SUPPLY("vci", "s6d05a-lcd")
};

static struct regulator_init_data spica_ldo7_data = {
	.constraints	= {
		.name			= "VLCD_3.0V",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= 0,
		.valid_ops_mask 	= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo7_consumer),
	.consumer_supplies	= ldo7_consumer,
};

static struct regulator_consumer_supply ldo8_consumer[] = {
	REGULATOR_SUPPLY("otg-core", "s3c-hsotg")
};

static struct regulator_init_data spica_ldo8_data = {
	.constraints	= {
		.name			= "VAP_OTG_3.3V",
		.min_uV			= 3300000,
		.max_uV			= 3300000,
		.apply_uV		= 0,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo8_consumer),
	.consumer_supplies	= ldo8_consumer,
};

static struct regulator_init_data spica_ldo9_data = {
	.constraints	= {
		.name			= "VAP_SYS_3.0V",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.enabled = 1,
		},
	},
};
*/
static struct regulator_consumer_supply buck_mem_consumer[] = {
	{	.supply	= "vddarm", },
};

static struct regulator_init_data jet_buck_mem_data = {
	.constraints	= {
		.name			= "VCORE_1.2V_S3C",
		.min_uV			= 1050000,
		.max_uV			= 1300000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck_mem_consumer),
	.consumer_supplies	= buck_mem_consumer,
};
/*
static struct regulator_consumer_supply buck2_consumer[] = {
	{	.supply	= "vddint", },
};

static struct regulator_init_data spica_buck2_data = {
	.constraints	= {
		.name			= "VAP_CORE_1.3V",
		.min_uV			= 750000,
		.max_uV			= 1500000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE |
					  REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck2_consumer),
	.consumer_supplies	= buck2_consumer,
};

static struct regulator_init_data spica_buck3_data = {
	.constraints	= {
		.name			= "VAP_MEM_1.8V",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= 0,
		.always_on		= 1,
		.valid_ops_mask		= REGULATOR_CHANGE_STATUS,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.state_mem		= {
			.enabled = 1,
		},
	},
};
*/

static struct max8906_regulator_data jet_regulators[] = {
	// Linear Regulators (LDOs)
/*
	// Two 30mA LDOs, Input Voltage Protected up to 28V Input
	{ MAX8906_LDO_VBUS,	&jet_vbus_data },
	{ MAX8906_LDO_USBTXRX,	&jet_usbtxrx_data },
	// Four 150mA Low-Noise LDOs, 60dB PSRR @ 100kHz, 45μV RMS Noise from 10Hz to 100kHz, 40μA Iq, Input Voltage Down to 1.7V
	{ MAX8906_LDO_RFRXL,	&jet_rfrxl_data },
	{ MAX8906_LDO_RFRXH,	&jet_rfrxh_data },
	{ MAX8906_LDO_RFTXL,	&jet_rftxl_data },
	{ MAX8906_LDO_SIMLT,	&jet_simlt_data },
	// Six 150mA Low Quiescent Current LDOs, 60dB PSRR @ 100kHz, 67μV RMS Noise from 10Hz to 100kHz, 25μA Iq, Input Voltage Down to 1.7V
*/
	{ MAX8906_LDOA,		&jet_ldoa_data },
/*
	{ MAX8906_LDOB,		&jet_ldob_data },
	{ MAX8906_LDOC,		&jet_ldoc_data },
	{ MAX8906_LDOD,		&jet_ldod_data },
	{ MAX8906_LDO_BIAS,	&jet_bias_data },
	{ MAX8906_LDO_RFTCXO,	&jet_rftcxo_data },
	// Seven 300mA LDOs, 60dB PSRR @ 100kHz, 67μV RMS Noise from 10Hz to 100kHz, 30μA Iq, Input Voltage Down to 1.7V
	{ MAX8906_LDO_MVT,	&jet_mvt_data },
	{ MAX8906_LDO_SRAM,	&jet_sram_data },
	{ MAX8906_LDO_CARD1,	&jet_card1_data },
	{ MAX8906_LDO_CARD2,	&jet_card2_data },
	{ MAX8906_LDO_WBBANA,	&jet_wbbana_data },
	{ MAX8906_LDO_WBBIO,	&jet_wbbio_data },
	{ MAX8906_LDO_WBBMEM,	&jet_wbbmem_data },
	// Step down regulators (DCDC BUCK)
	{ MAX8906_BUCK_WBBCORE,	&jet_wbbcore_data },
	{ MAX8906_BUCK_WBBRF,	&jet_wbbrf_data },
	{ MAX8906_BUCK_APPS,	&jet_apps_data },
	{ MAX8906_BUCK_IO,	&jet_io_data },
*/
	{ MAX8906_BUCK_MEM,	&jet_buck_mem_data }

};

static struct max8906_platform_data jet_max8906_pdata = {
	.regulators	= jet_regulators,
	.num_regulators	= ARRAY_SIZE(jet_regulators),
	.lbhyst		= 0, /* 100 mV */
	.lbth		= 2, /* 3,3 V */
	.lben		= 1, /* Enable low battery alarm */
};


static struct i2c_board_info jet_pmic_i2c_devs[] __initdata = {
		{
			.type		= "max8906",
			.addr		= 0x68,
			.platform_data	= &jet_max8906_pdata,
		},
		{
			.type		= "max8906",
			.addr		= 0x47,
			.platform_data	= &jet_max8906_pdata,
		},
		{
			.type		= "max8906",
			.addr		= 0x3C,
			.platform_data	= &jet_max8906_pdata,
		},
		{
			.type		= "max8906",
			.addr		= 0x34,
			.platform_data	= &jet_max8906_pdata,
		},
};


/* I2C 5 (GPIO) - Audio codec, VGACam
 * 	MAX8906 (Audio Amp)
 */
static struct i2c_gpio_platform_data jet_audio_i2c_pdata = {
	.sda_pin		= GPIO_AP_SDA_1_8V,
	.scl_pin		= GPIO_AP_SCL_1_8V,
	.udelay			= 6, /* 83,3KHz */
};

static struct platform_device jet_audio_i2c = {
	.name			= "i2c-gpio",
	.id			= 5,
	.dev.platform_data	= &jet_audio_i2c_pdata,
};

static struct max8906_codec_platform_data jet_max8906_codec_pdata = {
	.gpio_ampen = GPIO_MAX8906_AMP_EN,
};

static struct i2c_board_info jet_audio_i2c_devs[] __initdata = {
	{
		.type		= "max9880",
		.addr		= 0x10,
	}, {
			.type		= "max8906-codec",
			.addr		= 0x3c,
			.platform_data	= &jet_max8906_codec_pdata,
	},
};

struct platform_device sec_device_rtc = {
	.name   = "rtc-samsungjet",
	.id		= -1,
};
/*
 * Memory configuration
 */

#define PHYS_SIZE			(208*1024*1024)

#define RAM_CONSOLE_SIZE		(1*1024*1024)
#define PMEM_GPU1_SIZE			(32*1024*1024)
#define PMEM_SIZE			(16*1024*1024)

#define RESERVED_SIZE			(RAM_CONSOLE_SIZE \
					+ PMEM_GPU1_SIZE \
					+ PMEM_SIZE)

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name		= "pmem",
	.allocator_type	= PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached		= 1,
	.size		= PMEM_SIZE,
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name		= "pmem_gpu1",
	.allocator_type	= PMEM_ALLOCATORTYPE_BITMAP,
	.cached		= 1,
	.size		= PMEM_GPU1_SIZE,
};

static struct platform_device pmem_device = {
	.name		= "android_pmem",
	.id		= 0,
	.dev		= { .platform_data = &pmem_pdata },
};

static struct platform_device pmem_gpu1_device = {
	.name		= "android_pmem",
	.id		= 1,
	.dev		= { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device *pmem_devices[] = {
	&pmem_device,
	&pmem_gpu1_device,
};

static void __init spica_add_pmem_devices(void)
{
	unsigned i;
	for (i = 0; i < ARRAY_SIZE(pmem_devices); ++i)
		if (pmem_devices[i]->dev.platform_data) {
			struct android_pmem_platform_data *pmem =
					pmem_devices[i]->dev.platform_data;

			if (pmem->size)
				platform_device_register(pmem_devices[i]);
		}
}
#else
static void __init spica_add_pmem_devices(void) {}
#endif

static void __init spica_reserve(void)
{
	unsigned long start = PHYS_OFFSET + PHYS_SIZE - RESERVED_SIZE;
	unsigned long size = RESERVED_SIZE;
	struct mem_pool *mpool;
	int ret;

	memory_pool_init();

	ret = memblock_remove(start, size);
	WARN_ON(ret);

	mpool = initialize_memory_pool(start, size, 0);
	if (!mpool)
		pr_warning("failed to create mempool\n");
}


/*
 * LCD screen
 */

static struct ams310fn07_platform_data jet_ams310fn07_pdata = {
	.reset_gpio	= GPIO_LCD_RST_N,
	.cs_gpio	= GPIO_LCD_CS_N,
	.sck_gpio	= GPIO_LCD_SCLK,
	.sda_gpio	= GPIO_LCD_SDI,
};

static struct platform_device jet_ams310fn07 = {
	.name		= "ams310fn07-lcd",
	.id		= -1,
	.dev		= {
		.platform_data	= &jet_ams310fn07_pdata,
		.parent		= &s3c_device_fb.dev
	},
};

/*
 * SDHCI platform data
 */

static void jet_setup_sdhci_cfg_card(struct platform_device *dev,
				    void __iomem *r,
				    struct mmc_ios *ios,
				    struct mmc_card *card)
{
	u32 ctrl2 = 0, ctrl3 = 0;

	/* don't need to alter anything acording to card-type */

	writel(S3C64XX_SDHCI_CONTROL4_DRIVE_4mA, r + S3C64XX_SDHCI_CONTROL4);

	ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
	ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
		S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
		//S3C_SDHCI_CTRL2_ENFBCLKRX |
		S3C_SDHCI_CTRL2_DFCNT_NONE |
		S3C_SDHCI_CTRL2_ENCLKOUTHOLD);

	if (ios->clock < 25 * 1000000)
	{
		ctrl3 = (S3C_SDHCI_CTRL3_FCSEL3 |
			 S3C_SDHCI_CTRL3_FCSEL2 |
			 S3C_SDHCI_CTRL3_FCSEL1 |
			 S3C_SDHCI_CTRL3_FCSEL0);
	}
	else
	{
		ctrl3 = S3C_SDHCI_CTRL3_FCSEL0;
	}
	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

static struct s3c_sdhci_platdata jet_hsmmc0_pdata = {
	.max_width		= 4,
	.host_caps		= MMC_CAP_4_BIT_DATA
				| MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= GPIO_TF_DETECT,
	.ext_cd_gpio_invert	= 1,
	.cfg_card		= jet_setup_sdhci_cfg_card,
};

static struct s3c_sdhci_platdata jet_hsmmc1_pdata = {
	.max_width		= 4,
	.host_caps		= MMC_CAP_4_BIT_DATA |
				MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED |
				MMC_CAP_DISABLE,
	.cd_type		= S3C_SDHCI_CD_PERMANENT,
};

static int spica_wlan_cd_state = 0;
static void (*spica_wlan_notify_func)(struct platform_device *, int) = 0;

static int spica_wlan_cd_init(void (*func)(struct platform_device *, int state))
{
	printk("%s\n", __func__);
	spica_wlan_notify_func = func;

	func(&s3c_device_hsmmc2, spica_wlan_cd_state);

	return 0;
}

static int spica_wlan_cd_cleanup(void (*notify_func)(struct platform_device *,
								int state))
{
	printk("%s\n", __func__);
	spica_wlan_notify_func = NULL;

	return 0;
}

void jet_setup_sdhci2_cfg_gpio(struct platform_device *dev, int width)
{
	/* Nothing to do here */
}

static struct s3c_sdhci_platdata spica_hsmmc2_pdata = {
	.max_width		= 4,
	.host_caps		= MMC_CAP_4_BIT_DATA
				| MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
	.cd_type		= S3C_SDHCI_CD_EXTERNAL,
	.ext_cd_init		= spica_wlan_cd_init,
	.ext_cd_cleanup		= spica_wlan_cd_cleanup,
	.built_in		= 1,
	.cfg_card		= jet_setup_sdhci_cfg_card,
	.cfg_gpio               = jet_setup_sdhci2_cfg_gpio,
};

static struct regulator_consumer_supply mmc2_supplies[] = {
	REGULATOR_SUPPLY("vmmc", "s3c-sdhci.2"),
};

static struct regulator_init_data mmc2_fixed_voltage_init_data = {
	.constraints		= {
		.name		= "WLAN_VDD_2.8V",
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(mmc2_supplies),
	.consumer_supplies	= mmc2_supplies,
};

static struct fixed_voltage_config mmc2_fixed_voltage_config = {
	.supply_name		= "WLAN_REG",
	.microvolts		= 2800000,
	.init_data		= &mmc2_fixed_voltage_init_data,
	.gpio			= -EINVAL,
};

static struct platform_device mmc2_fixed_voltage = {
	.name			= "reg-fixed-voltage",
	.id			= 0,
	.dev			= {
		.platform_data	= &mmc2_fixed_voltage_config,
	},
};

/*
 * Framebuffer
 */
//#define SECOND_FB 1

static struct s3c_fb_pd_win jet_fb_win[] = {
	[0] = {
		.win_mode	= {
			.left_margin	= 64,
			.right_margin	= 62,
			.upper_margin	= 8,
			.lower_margin	= 6,
			.hsync_len	= 2,
			.vsync_len	= 2,
			.xres		= 480,
			.yres		= 800,
		},
		.max_bpp	= 24,
		.default_bpp	= 16,
		.virtual_y	= 800 * 2,
		.virtual_x	= 480,
	},
#ifdef SECOND_FB
	[1] = {
		.win_mode	= {
			.left_margin	= 64,
			.right_margin	= 62,
			.upper_margin	= 8,
			.lower_margin	= 6,
			.hsync_len	= 2,
			.vsync_len	= 2,
			.xres		= 480,
			.yres		= 800,
		},
		.max_bpp	= 24,
		.default_bpp	= 16,
		.virtual_y	= 800 * 2,
		.virtual_x	= 480,
	},
#endif
};

static void jet_fb_setup_gpio(void)
{
	/* Nothing to do here */
}

static struct s3c_fb_platdata jet_lcd_pdata __initdata = {
	.setup_gpio	= jet_fb_setup_gpio,
	.win[0]		= &jet_fb_win[0],
#ifdef SECOND_FB
	.win[1]		= &jet_fb_win[1],
#endif
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
//			| VIDCON0_CLKSEL_LCD,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC
			| VIDCON1_INV_VDEN | VIDCON1_INV_VCLK,
	.dithmode	= DITHMODE_R_POS_8BIT | DITHMODE_G_POS_8BIT
			| DITHMODE_B_POS_8BIT | DITHMODE_DITH_EN,
};

/*
 * RAM console (for debugging)
 */

static struct resource spica_ram_console_resources[] = {
	{
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device spica_ram_console = {
	.name		= "ram_console",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(spica_ram_console_resources),
	.resource	= spica_ram_console_resources,
};

/*
 * Matrix keyboard (FIXME: Assign standard key codes)
 */

static uint32_t jet_keymap[] __initdata = {
	/* KEY(row, col, keycode) */
	KEY(0, 0, 1), KEY(0, 1, 9), KEY(0, 2, 17),
	KEY(1, 0, 2), KEY(1, 1, 10), /* Reserved  ,*/
	KEY(2, 0, 3), KEY(2, 1, 11), KEY(2, 2, 19),
};

static struct matrix_keymap_data jet_keymap_data __initdata = {
	.keymap		= jet_keymap,
	.keymap_size	= ARRAY_SIZE(jet_keymap),
};

static struct samsung_keypad_platdata jet_keypad_pdata __initdata = {
	.keymap_data	= &jet_keymap_data,
	.rows		= 3,
	.cols		= 3,
	.no_autorepeat	= 1,
	.wakeup		= 1,
};

/*
 * GPIO keys (FIXME: Assign standard key codes)
 */

static struct gpio_keys_button jet_gpio_keys_data[] = {
	{
		.gpio			= GPIO_POWER_N,
		.code			= 249,
		.desc			= "Power",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type                   = EV_KEY,
		.wakeup			= 1,
	},
};

static struct gpio_keys_platform_data jet_gpio_keys_pdata  = {
	.buttons	= jet_gpio_keys_data,
	.nbuttons	= ARRAY_SIZE(jet_gpio_keys_data),
};

static struct platform_device jet_gpio_keys = {
	.name		= "gpio-keys",
	.id		= 0,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &jet_gpio_keys_pdata,
	}
};

/*
 * OneNAND
 */

static struct mtd_partition spica_onenand_parts[] = {
	/*
	 * Bootloaders (DO NOT TOUCH!)
	 */
	[0] = {
		.name		= "pbl",
		.size		= SZ_128K,
		.offset		= 0x00000000,
		.mask_flags	= MTD_WRITEABLE,
	},
	[1] = {
		.name		= "sbl",
		.size		= SZ_1M + SZ_256K,
		.offset		= 0x00020000,
		.mask_flags	= MTD_WRITEABLE,
	},
	/*
	 * Bootloader parameters
	 */
	[2] = {
		.name		= "logo",
		.size		= SZ_128K,
		.offset		= 0x00160000,
	},
	[3] = {
		.name		= "param",
		.size		= SZ_256K,
		.offset		= 0x00180000,
		.mask_flags	= MTD_USERSPACE,
	},
	/*
	 * Kernel (zImage)
	 */
	[4] = {
		.name		= "kernel",
		.size		= SZ_4M + SZ_1M,
		.offset		= 0x001c0000,
	},
	/*
	 * User accessible partitions (incompatible with legacy kernels)
	 */
	[5] = {
		.name		= "system",
		.size		= SZ_128M + SZ_16M + SZ_8M,
		.offset		= 0x006c0000,
	},
	[6] = {
		.name		= "data",
		.size		= SZ_256M + SZ_16M + SZ_4M + SZ_2M,
		.offset		= 0x09ec0000,
	},
	[7] = {
		.name		= "xbin",
		.size		= SZ_16M + SZ_8M,
		.offset		= 0x1b540000,
	},
	[8] = {
		.name		= "cache",
		.size		= SZ_8M,
		.offset		= 0x1cd40000,
	},
	[9] = {
		.name		= "efs",
		.size		= SZ_8M,
		.offset		= 0x1d540000,
	},
	/*
	 * Baseband firmware
	 */
	[10] = {
		.name		= "modem",
		.size		= SZ_16M,
		.offset		= 0x1dd40000,
	},
	/*
	 * EFS partition of legacy kernels, unused
	 */
	[11] = {
		.name		= "efs_legacy",
		.size		= SZ_8M,
		.offset		= 0x1ed40000,
		.mask_flags	= MTD_WRITEABLE,
	},
	/*
	 * XSR reservoir
	 */
	[12] = {
		.name		= "reservoir",
		.size		= SZ_8M + SZ_2M + SZ_512K + SZ_128K,
		.offset		= 0x1f540000,
		.mask_flags	= MTD_USERSPACE | MTD_UNLOCKED,
	},
	/*
	 * Phone unique data
	 */
	[13] = {
		.name		= "dgs",
		.size		= SZ_128K,
		.offset		= 0x1ffe0000,
		.mask_flags	= MTD_WRITEABLE,
	},
};

static struct onenand_platform_data spica_onenand_pdata = {
	.parts		= spica_onenand_parts,
	.nr_parts	= ARRAY_SIZE(spica_onenand_parts),
};

static struct bml_partition spica_bml_parts[] = {
	{
		.name	= "kernel",
		.offset	= 0x001c0000,
	},
	{
		.name	= "xbin",
		.offset	= 0x1b540000,
	},
	{
		.name	= "modem",
		.offset	= 0x1dd40000,
	},
};

static struct bml_platform_data spica_bml_pdata = {
	.parts		= spica_bml_parts,
	.nr_parts	= ARRAY_SIZE(spica_bml_parts),
};

static struct platform_device spica_bml_device = {
	.name	= "mtd-bml",
	.id	= -1,
	.dev	= {
		.platform_data = &spica_bml_pdata,
	},
};

/*
 * Hardware monitoring (ADC)
 */

/* TODO: Verify these values. */
#if 0
static struct spica_battery_threshold spica_battery_percent_lut[] = {
	/* ADC, 0.001% */
	{ 2170,      0 },
	{ 2310,      0 },
	{ 2311,   1000 },
	{ 2419,   3000 },
	{ 2455,   5000 },
	{ 2490,  15000 },
	{ 2513,  30000 },
	{ 2559,  50000 },
	{ 2635,  70000 },
	{ 2800, 100000 },
};

static struct spica_battery_threshold spica_battery_volt_lut[] = {
	/* ADC, microvolts */
	{ 2170, 3200000 },
	{ 2240, 3300000 },
	{ 2310, 3400000 },
	{ 2419, 3554000 },
	{ 2455, 3603700 },
	{ 2490, 3656600 },
	{ 2513, 3690700 },
	{ 2559, 3755400 },
	{ 2635, 3864100 },
	{ 2800, 4100000 },
	{ 2853, 4176000 },
	{ 3080, 4500000 },
};
#endif

static struct jet_battery_threshold jet_battery_temp_lut[] = {
	/* ADC, 0.001*C */
	{  324,  65000 },
	{  364,  61000 },
	{  388,  59000 },
	{  439,  55000 },
	{  511,  50000 },
	{  593,  45000 },
	{  684,  40000 },
	{  804,  34000 },
	{  845,  32000 },
	{  886,  30000 },
	{  994,  25000 },
	{ 1112,  20000 },
	{ 1224,  15000 },
	{ 1340,  10000 },
	{ 1450,   5000 },
	{ 1550,      0 },
	{ 1632,  -5000 },
	{ 2042, -30000 },
};

static void jet_charger_supply_detect_init(jet_battery_notify_func_t *func)
{
	jet_battery_notify_func = func;
	jet_battery_notify();
}

static void jet_charger_supply_detect_cleanup(void)
{
	jet_battery_notify_func = NULL;
}

static struct jet_battery_pdata jet_battery_pdata = {
	.gpio_pok		= GPIO_TA_SEL,
	.gpio_pok_inverted	= 1,
	.gpio_chg		= GPIO_PMIC_INT_N,
	.gpio_chg_inverted	= 1,
//	.gpio_en		= GPIO_TA_EN,
//	.gpio_en_inverted	= 1,

//	.percent_lut		= spica_battery_percent_lut,
//	.percent_lut_cnt	= ARRAY_SIZE(spica_battery_percent_lut),
//	.volt_lut		= spica_battery_volt_lut,
//	.volt_lut_cnt		= ARRAY_SIZE(spica_battery_volt_lut),
	.temp_lut		= jet_battery_temp_lut,
	.temp_lut_cnt		= ARRAY_SIZE(jet_battery_temp_lut),

//	.calibration		= 2447,

	.volt_channel		= 0,
	.temp_channel		= 2,

	.low_temp_enter		= 0,
	.low_temp_exit		= 2000,
	.high_temp_exit		= 50000,
	.high_temp_enter	= 60000,

	.technology		= POWER_SUPPLY_TECHNOLOGY_LION,

	.supply_detect_init	= jet_charger_supply_detect_init,
	.supply_detect_cleanup	= jet_charger_supply_detect_cleanup,
};

static struct platform_device jet_battery = {
	.name		= "jet-battery",
	.id		= -1,
	.dev		= {
		.platform_data	= &jet_battery_pdata,
	}
};

/*
 * Common WLAN/Bluetooth power switch
 */

static int spica_wifi_bt_pwr_cnt = 0;
static DEFINE_MUTEX(spica_wifi_bt_pwr_lock);

static void spica_wifi_bt_power_inc(void)
{
	mutex_lock(&spica_wifi_bt_pwr_lock);

	if (!(spica_wifi_bt_pwr_cnt++)) {
		printk(KERN_INFO "WiFi/BT power ON.\n");
		gpio_set_value(GPIO_BT_WLAN_REG_ON, 1);
	}

	mutex_unlock(&spica_wifi_bt_pwr_lock);
}

static void spica_wifi_bt_power_dec(void)
{
	mutex_lock(&spica_wifi_bt_pwr_lock);

	if (!(--spica_wifi_bt_pwr_cnt)) {
		printk(KERN_INFO "WiFi/BT power OFF.\n");
		gpio_set_value(GPIO_BT_WLAN_REG_ON, 0);
		msleep(100);
	}

	mutex_unlock(&spica_wifi_bt_pwr_lock);
}

/*
 * Bluetooth
 */

static struct s3c_pin_cfg_entry jet_bt_pin_config_on[] = {
	S3C_PIN(GPIO_WLAN_HOST_WAKE), S3C_PIN_PULL(NONE),
	S3C64XX_GPA4_UART1_RXD, S3C_PIN_PULL(NONE),
	S3C64XX_GPA5_UART1_TXD, S3C_PIN_PULL(NONE),
	S3C64XX_GPA6_UART1_CTSN, S3C_PIN_PULL(NONE),
	S3C64XX_GPA7_UART1_RTSN, S3C_PIN_PULL(NONE),
};

static struct s3c_pin_cfg_entry jet_bt_pin_config_off[] = {
	S3C_PIN(GPIO_WLAN_HOST_WAKE), S3C_PIN_PULL(DOWN),
	S3C64XX_PIN(GPA(4)), S3C_PIN_IN, S3C_PIN_PULL(DOWN),
	S3C64XX_PIN(GPA(5)), S3C_PIN_IN, S3C_PIN_PULL(DOWN),
	S3C64XX_PIN(GPA(6)), S3C_PIN_IN, S3C_PIN_PULL(DOWN),
	S3C64XX_PIN(GPA(7)), S3C_PIN_IN, S3C_PIN_PULL(DOWN),
};

static int spica_bt_power = 0;

static void spica_bt_set_power(int val)
{
	if (val == spica_bt_power)
		return;

	if (val) {
		gpio_set_value(GPIO_BT_RST_N, 0);
		spica_wifi_bt_power_inc();
		msleep(100);
		gpio_set_value(GPIO_BT_RST_N, 1);
		msleep(50);
		s3c_pin_config(jet_bt_pin_config_on,
					ARRAY_SIZE(jet_bt_pin_config_on));
	} else {
		s3c_pin_config(jet_bt_pin_config_off,
					ARRAY_SIZE(jet_bt_pin_config_off));
		gpio_set_value(GPIO_BT_RST_N, 0);
		spica_wifi_bt_power_dec();
	}

	spica_bt_power = val;
}

static struct spica_bt_pdata spica_bt_pdata = {
	.gpio_host_wake	= GPIO_BT_HOST_WAKE,
	.set_power	= spica_bt_set_power,
};

static struct platform_device spica_bt_device = {
	.name	= "spica_bt",
	.dev	= {
		.platform_data = &spica_bt_pdata,
	},
};

static struct hrtimer spica_bt_lpm_timer;
static ktime_t spica_bt_lpm_delay;

static enum hrtimer_restart spica_bt_enter_lpm(struct hrtimer *timer)
{
	gpio_set_value(GPIO_BT_WAKE, 0);

	return HRTIMER_NORESTART;
}

static void spica_bt_uart_wake_peer(struct uart_port *port)
{
	if (!spica_bt_lpm_timer.function)
		return;

	hrtimer_try_to_cancel(&spica_bt_lpm_timer);
	gpio_set_value(GPIO_BT_WAKE, 1);
	hrtimer_start(&spica_bt_lpm_timer,
					spica_bt_lpm_delay, HRTIMER_MODE_REL);
}

static void __init spica_bt_lpm_init(void)
{
	WARN_ON(gpio_request(GPIO_BT_WAKE, "gpio_bt_wake") < 0);

	gpio_direction_output(GPIO_BT_WAKE, 0);

	hrtimer_init(&spica_bt_lpm_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	spica_bt_lpm_timer.function = spica_bt_enter_lpm;
	spica_bt_lpm_delay = ktime_set(1, 0);	/* 1 sec */
}

/*
 * WLAN (using bcmdhd driver)
 */

static int spica_wlan_power = 0;
static struct wake_lock wlan_wakelock;

static int spica_wlan_set_power(int val)
{
	printk("%s = %d\n", __func__, val);

	if (val == spica_wlan_power)
		return 0;

	if (val) {
		wake_lock(&wlan_wakelock);
		gpio_set_value(GPIO_WLAN_RST_N, 0);
		spica_wifi_bt_power_inc();
		msleep(150);
		gpio_set_value(GPIO_WLAN_RST_N, 1);
		s3c_gpio_setpull(GPIO_WLAN_HOST_WAKE, S3C_GPIO_PULL_NONE);
	} else {
		s3c_gpio_setpull(GPIO_WLAN_HOST_WAKE, S3C_GPIO_PULL_DOWN);
		gpio_set_value(GPIO_WLAN_RST_N, 0);
		spica_wifi_bt_power_dec();
		wake_unlock(&wlan_wakelock);
	}

	spica_wlan_power = val;

	return 0;
}

static int spica_wlan_set_reset(int val)
{
	printk("%s = %d\n", __func__, val);
	gpio_set_value(GPIO_WLAN_RST_N, !val);
	return 0;
}

static int spica_wlan_set_carddetect(int val)
{
	printk("%s = %d\n", __func__, val);
	spica_wlan_cd_state = val;
	if (spica_wlan_notify_func)
		spica_wlan_notify_func(&s3c_device_hsmmc2, val);
	msleep(100);
	return 0;
}

static struct wifi_platform_data spica_wlan_pdata = {
	.set_power		= spica_wlan_set_power,
	.set_reset		= spica_wlan_set_reset,
	.set_carddetect		= spica_wlan_set_carddetect,
};

static struct resource spica_wlan_resources[] = {
	{
		.name	= "bcmdhd_wlan_irq",
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL,
		.start	= IRQ_WLAN,
		.end	= IRQ_WLAN,
	},
};

static struct platform_device spica_wlan_device = {
	.name		= "bcmdhd_wlan",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(spica_wlan_resources),
	.resource	= spica_wlan_resources,
	.dev		= {
		.platform_data	= &spica_wlan_pdata,
	},
};

/*
 * USB gadget
 */

#define S3C_VENDOR_ID			0x18d1
#define S3C_UMS_PRODUCT_ID		0x4E21
#define S3C_UMS_ADB_PRODUCT_ID		0x4E22
#define S3C_MTP_PRODUCT_ID		0x4E21
#define S3C_MTP_ADB_PRODUCT_ID		0x4E22
#define S3C_RNDIS_PRODUCT_ID		0x4E23
#define S3C_RNDIS_ADB_PRODUCT_ID	0x4E24

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};

#ifdef CONFIG_USB_ANDROID_ACCESSORY
static char *usb_functions_accessory[] = {
	"accessory",
};

static char *usb_functions_accessory_adb[] = {
	"accessory",
	"adb",
};
#endif

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

#ifdef CONFIG_USB_ANDROID_MTP
static char *usb_functions_mtp[] = {
	"mtp",
};

static char *usb_functions_mtp_adb[] = {
	"mtp",
	"adb",
};
#endif

static char *usb_functions_all[] = {
	"rndis",
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	"accessory",
#endif
	"usb_mass_storage",
#ifdef CONFIG_USB_ANDROID_MTP
	"mtp",
#endif
	"adb",
};

static struct android_usb_product usb_products[] = {
	{
		.product_id	= S3C_UMS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	}, {
		.product_id	= S3C_UMS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
#ifdef CONFIG_USB_ANDROID_MTP
	{
		.product_id	= S3C_MTP_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
	}, {
		.product_id	= S3C_MTP_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_adb),
		.functions	= usb_functions_mtp_adb,
	},
#endif
	{
		.product_id	= S3C_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	}, {
		.product_id	= S3C_RNDIS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	{
		.vendor_id	= USB_ACCESSORY_VENDOR_ID,
		.product_id	= USB_ACCESSORY_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_accessory),
		.functions	= usb_functions_accessory,
	}, {
		.vendor_id	= USB_ACCESSORY_VENDOR_ID,
		.product_id	= USB_ACCESSORY_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_accessory_adb),
		.functions	= usb_functions_accessory_adb,
	},
#endif
};

static char device_serial[] = "GTS80000123456789";
/* standard android USB platform data */

/* Information should be changed as real product for commercial release */
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_UMS_PRODUCT_ID,
	.manufacturer_name	= "Samsung",
	.product_name		= "GT-S8000",
	.serial_number		= device_serial,
	.num_products		= ARRAY_SIZE(usb_products),
	.products		= usb_products,
	.num_functions		= ARRAY_SIZE(usb_functions_all),
	.functions		= usb_functions_all,
};

static struct platform_device spica_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};

static struct usb_mass_storage_platform_data ums_pdata = {
	.vendor			= "Android",
	.product		= "UMS Composite",
	.release		= 1,
	.nluns			= 1,
};

static struct platform_device spica_usb_mass_storage = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};

static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x18d1,
	.vendorDescr	= "Samsung",
};

static struct platform_device spica_usb_rndis = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

/*
 * Telephony modules
 */

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

#define JET_DPRAM_START		0x5d000000
#define JET_DPRAM_SIZE		SZ_16M

static struct resource jet_dpram_resources[] = {
	{
		.start	= JET_DPRAM_START,
		.end	= JET_DPRAM_START + JET_DPRAM_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	}
};

struct dpram_platform_data {
	unsigned int gpio_phone_on;
	unsigned int gpio_phone_rst_n;
	unsigned int gpio_phone_active;
	unsigned int gpio_cp_boot_sel;
	unsigned int gpio_usim_boot;
	unsigned int gpio_pda_active;
	unsigned int gpio_onedram_int_n;
	unsigned int gpio_sim_detect_n;
};

static struct dpram_platform_data jet_dpram_pdata = {
	.gpio_phone_on		= GPIO_PHONE_ON,
//	.gpio_phone_rst_n	= GPIO_PHONE_RST_N,	// FIXME: This GPIO is handeled via max8906
	.gpio_phone_active	= GPIO_PHONE_ACTIVE,
	.gpio_cp_boot_sel	= GPIO_CP_BOOT_SEL,
	.gpio_usim_boot		= GPIO_USIM_BOOT,
	.gpio_pda_active	= GPIO_PDA_ACTIVE,
	.gpio_onedram_int_n	= GPIO_ONEDRAM_INT_N,
//	.gpio_sim_detect_n	= GPIO_SIM_DETECT_N,	// FIXME: How to detect SIM?
};

static struct platform_device jet_dpram_device = {
	.name		= "jet-dpram",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(jet_dpram_resources),
	.resource	= jet_dpram_resources,
	.dev		= {
		.platform_data = &jet_dpram_pdata,
	},
};

/*
 * Vibetonz
 */
static struct vibetonz_platform_data spica_vibetonz_pdata = {
	.gpio_en	= GPIO_VIB_EN,
	.pwm_chan	= 1,
};

static struct platform_device spica_vibetonz_device = {
	.name	= "vibetonz",
	.id	= -1,
	.dev	= {
		.platform_data = &spica_vibetonz_pdata,
	},
};

/*
 * Sound
 */
static bool snd_mic_bias = 0;
static bool jack_mic_bias = 0;

static void snd_set_mic_bias(bool on)
{
	unsigned long flags;

	local_irq_save(flags);

	snd_mic_bias = on;
//	gpio_set_value(GPIO_MICBIAS_EN, snd_mic_bias || jack_mic_bias);
// FIXME: enable MICBIAS_EAR_LDO_2.5V via MAX8906
	//if (Set_MAX8906_PM_REG(WMEMEN, snd_mic_bias || jack_mic_bias))
		printk("Successfully enabled regulator WMEMEN\n");

	local_irq_restore(flags);
}

static void jack_set_mic_bias(bool on)
{
	unsigned long flags;
	local_irq_save(flags);

	jack_mic_bias = on;
	if (Set_MAX8906_PM_REG(WMEMEN, snd_mic_bias || jack_mic_bias))
		printk("Changed Ear Mic Bias to %d\n", snd_mic_bias || jack_mic_bias);

	local_irq_restore(flags);
}

static struct sec_jack_zone spica_jack_zones[] = {
	{
		/* adc == 0, unstable zone, default to 3pole if it stays
		 * in this range for a half second (20ms delays, 25 samples)
		 */
		.adc_high = 0,
		.delay_ms = 20,
		.check_count = 25,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 0 < adc <= 1000, unstable zone, default to 3pole if it stays
		 * in this range for a second (10ms delays, 100 samples)
		 */
		.adc_high = 1000,
		.delay_ms = 10,
		.check_count = 100,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 1000 < adc <= 2000, unstable zone, default to 4pole if it
		 * stays in this range for a second (10ms delays, 100 samples)
		 */
		.adc_high = 2000,
		.delay_ms = 10,
		.check_count = 100,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* 2000 < adc <= 3700, 4 pole zone, default to 4pole if it
		 * stays in this range for 200ms (20ms delays, 10 samples)
		 */
		.adc_high = 3700,
		.delay_ms = 20,
		.check_count = 10,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* adc > 3700, unstable zone, default to 3pole if it stays
		 * in this range for a second (10ms delays, 100 samples)
		 */
		.adc_high = 0x7fffffff,
		.delay_ms = 10,
		.check_count = 100,
		.jack_type = SEC_HEADSET_3POLE,
	},
};

static struct sec_jack_platform_data spica_jack_pdata = {
	.set_micbias_state = jack_set_mic_bias,
	.adc_chan = 3,
	.zones = spica_jack_zones,
	.num_zones = ARRAY_SIZE(spica_jack_zones),
	.det_gpio = GPIO_DET_35,
	.send_end_gpio = GPIO_EAR_SEND_END,
	.det_active_high = 1,
	.send_end_active_high = 0,
};

static struct platform_device spica_jack_device = {
	.name	= "sec_jack",
	.id	= -1,
	.dev	= {
		.platform_data = &spica_jack_pdata,
	},
};

static struct gt_i5700_audio_pdata jet_audio_pdata = {
	.set_micbias	= snd_set_mic_bias,
};

static struct platform_device jet_audio_device = {
	.name	= "gt_s8000_audio",
	.id	= -1,
	.dev	= {
		.platform_data = &jet_audio_pdata,
	},
};

static int spica_i2s_cfg_gpio(struct platform_device *pdev)
{
	/* Nothing to do here */
	return 0;
}

static const char *spica_i2s_clksrc[] = {
	[0] = "iis",
	[1] = "audio-bus",
};

static struct s3c_audio_pdata spica_i2s_pdata = {
	.cfg_gpio = spica_i2s_cfg_gpio,
	.type = {
		.i2s = {
			.src_clk = spica_i2s_clksrc,
		},
	},
};

struct platform_device sec_device_battery = {
	.name   = "jet-battery",
	.id		= -1,
};

static struct s3c2410_ts_mach_info s3c_ts_platform __initdata = {
	.delay			= 30000,
	.presc			= 49,
	.oversampling_shift	= 3,
};

/*
 * Platform devices
 */

static struct platform_device *spica_devices[] __initdata = {
	&mmc2_fixed_voltage,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
	&s3c_device_hsmmc2,
	&sec_device_rtc,
	&s3c_device_i2c0,
	&s3c_device_fb,
	&jet_pmic_i2c,
	&jet_usbsw_i2c,
	&s3c_device_usb_hsotg,
	&spica_usb_rndis,
	&spica_usb_mass_storage,
	&spica_android_usb,
	&s3c_device_onenand,
	&samsung_device_keypad,
	&jet_fm_i2c,
	&jet_audio_i2c,
	&jet_ams310fn07,
	&spica_ram_console,
	&jet_gpio_keys,
	&s3c_device_adc,
	&s3c_device_ts,
	&jet_battery,
	//&sec_device_battery,
	&samsung_asoc_dma,
	&s3c64xx_device_iis0,
	&s3c_device_timer[1],
	&spica_wlan_device,
	&spica_bt_device,
	&jet_dpram_device,
	&spica_vibetonz_device,
	&spica_jack_device,
	//&spica_audio_device,
	&jet_audio_device,
	&spica_bml_device,
};

/*
 * Platform devices for Samsung modules (FIXME: Make this board-independent.)
 */

static struct resource s3c_g3d_resource[] = {
	[0] = {
		.start = S3C64XX_PA_G3D,
		.end   = S3C64XX_PA_G3D + SZ_16M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_S3C6410_G3D,
		.end   = IRQ_S3C6410_G3D,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_mfc_resource[] = {
	[0] = {
		.start = S3C64XX_PA_MFC,
		.end   = S3C64XX_PA_MFC + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_MFC,
		.end   = IRQ_MFC,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_jpeg_resource[] = {
	[0] = {
		.start = S3C64XX_PA_JPEG,
		.end   = S3C64XX_PA_JPEG + SZ_4M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_JPEG,
		.end   = IRQ_JPEG,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_camif_resource[] = {
	[0] = {
		.start = S3C64XX_PA_CAMIF,
		.end   = S3C64XX_PA_CAMIF + SZ_4M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_CAMIF_C,
		.end   = IRQ_CAMIF_C,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_CAMIF_P,
		.end   = IRQ_CAMIF_P,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_g2d_resource[] = {
	[0] = {
		.start = S3C64XX_PA_2D,
		.end   = S3C64XX_PA_2D + SZ_1M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_2D,
		.end   = IRQ_2D,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_rotator_resource[] = {
	[0] = {
		.start = S3C64XX_PA_ROTATOR,
		.end   = S3C64XX_PA_ROTATOR + SZ_1M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_ROTATOR,
		.end   = IRQ_ROTATOR,
		.flags = IORESOURCE_IRQ,
	}
};

static struct resource s3c_pp_resource[] = {
	[0] = {
		.start = S3C64XX_PA_POST0,
		.end   = S3C64XX_PA_POST0 + SZ_1M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_POST0,
		.end   = IRQ_POST0,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device *spica_mod_devices[] __initdata = {
	&(struct platform_device){
		.name		= "s3c-g3d",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_g3d_resource),
		.resource	= s3c_g3d_resource,
		.dev		= {
			.parent	= &s3c64xx_device_pd[S3C6410_DOMAIN_G].dev,
		},
	}, &(struct platform_device){
		.name		= "s3c-mfc",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_mfc_resource),
		.resource	= s3c_mfc_resource,
		.dev		= {
			.parent	= &s3c64xx_device_pd[S3C64XX_DOMAIN_V].dev,
		},
	}, &(struct platform_device){
		.name		= "s3c-jpeg",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_jpeg_resource),
		.resource	= s3c_jpeg_resource,
		.dev		= {
			.parent	= &s3c64xx_device_pd[S3C64XX_DOMAIN_I].dev,
		},
	}, &(struct platform_device){
		.name		= "s3c-fimc",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_camif_resource),
		.resource	= s3c_camif_resource,
		.dev		= {
			.parent	= &s3c64xx_device_pd[S3C64XX_DOMAIN_I].dev,
		},
	}, &(struct platform_device){
		.name		= "s3c-g2d",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_g2d_resource),
		.resource	= s3c_g2d_resource,
		.dev		= {
			.parent	= &s3c64xx_device_pd[S3C64XX_DOMAIN_P].dev,
		},
	}, &(struct platform_device){
		.name		= "s3c-rotator",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_rotator_resource),
		.resource	= s3c_rotator_resource,
		.dev		= {
			.parent	= &s3c64xx_device_pd[S3C64XX_DOMAIN_F].dev,
		},
	}, &(struct platform_device){
		.name		= "s3c-pp",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(s3c_pp_resource),
		.resource	= s3c_pp_resource,
		.dev		= {
			.parent	= &s3c64xx_device_pd[S3C64XX_DOMAIN_F].dev,
		},
	}
};

/*
 * Extended I/O map
 */

static struct map_desc spica_iodesc[] __initdata = {

};

/*
 * GPIO setup
 */

static struct s3c_pin_cfg_entry spica_pin_config[] __initdata = {
	/* UART 1 (Bluetooth) */
	S3C64XX_GPA4_UART1_RXD, S3C_PIN_PULL(NONE),
	S3C64XX_GPA5_UART1_TXD, S3C_PIN_PULL(NONE),
	S3C64XX_GPA6_UART1_CTSN, S3C_PIN_PULL(NONE),
	S3C64XX_GPA7_UART1_RTSN, S3C_PIN_PULL(NONE),

	/* UART 2 (External / USB SW) */
	S3C64XX_GPB0_UART2_RXD, S3C_PIN_PULL(NONE),
	S3C64XX_GPB1_UART2_TXD, S3C_PIN_PULL(NONE),

	/* UART 3 (Phone) */
	S3C64XX_GPB2_UART3_RXD, S3C_PIN_PULL(NONE), // old max-jet.c had PULL(DOWN)
	S3C64XX_GPB3_UART3_TXD, S3C_PIN_PULL(NONE), // old max-jet.c had PULL(DOWN)

	/* I2C 0 */
	S3C6410_GPB5_I2C0_SCL, S3C_PIN_PULL(NONE),
	S3C6410_GPB6_I2C0_SDA, S3C_PIN_PULL(NONE),

	/* MMC 2 (WLAN) */
	S3C64XX_GPC4_MMC2_CMD, S3C_PIN_PULL(NONE),
	S3C64XX_GPC5_MMC2_CLK, S3C_PIN_PULL(NONE),
	S3C64XX_GPH6_MMC2_DATA0, S3C_PIN_PULL(NONE),
	S3C64XX_GPH7_MMC2_DATA1, S3C_PIN_PULL(NONE),
	S3C64XX_GPH8_MMC2_DATA2, S3C_PIN_PULL(NONE),
	S3C64XX_GPH9_MMC2_DATA3, S3C_PIN_PULL(NONE),

	/* I2S 0 */
	S3C64XX_GPD0_I2S0_CLK, S3C_PIN_PULL(NONE),
	S3C64XX_GPD2_I2S0_LRCLK, S3C_PIN_PULL(NONE),
	S3C64XX_GPD3_I2S0_DI, S3C_PIN_PULL(NONE),
	S3C64XX_GPD4_I2S0_DO, S3C_PIN_PULL(NONE),

	/* PCM 1 */
	S3C64XX_GPE0_PCM1_SCLK, S3C_PIN_PULL(NONE),
	S3C64XX_GPE2_PCM1_FSYNC, S3C_PIN_PULL(NONE),
	S3C64XX_GPE3_PCM1_SIN, S3C_PIN_PULL(NONE),
	S3C64XX_GPE4_PCM1_SOUT, S3C_PIN_PULL(NONE),

	/* CAMIF */
	S3C64XX_GPF0_CAMIF_CLK, S3C_PIN_PULL(NONE),
	S3C64XX_GPF1_CAMIF_HREF, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF2_CAMIF_PCLK, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF4_CAMIF_VSYNC, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF5_CAMIF_YDATA0, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF6_CAMIF_YDATA1, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF7_CAMIF_YDATA2, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF8_CAMIF_YDATA3, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF9_CAMIF_YDATA4, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF10_CAMIF_YDATA5, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF11_CAMIF_YDATA6, S3C_PIN_PULL(DOWN),
	S3C64XX_GPF12_CAMIF_YDATA7, S3C_PIN_PULL(DOWN),

	/* PWM */
	S3C64XX_GPF15_PWM_TOUT1, S3C_PIN_PULL(NONE),

	/* MMC 0 (TF) */
	S3C64XX_GPG0_MMC0_CLK, S3C_PIN_PULL(DOWN),
	S3C64XX_GPG1_MMC0_CMD, S3C_PIN_PULL(NONE),
	S3C64XX_GPG2_MMC0_DATA0, S3C_PIN_PULL(NONE),
	S3C64XX_GPG3_MMC0_DATA1, S3C_PIN_PULL(NONE),
	S3C64XX_GPG4_MMC0_DATA2, S3C_PIN_PULL(NONE),
	S3C64XX_GPG5_MMC0_DATA3, S3C_PIN_PULL(NONE),

	/* MMC 1 (iNAND) */
	S3C64XX_GPH0_MMC1_CLK, S3C_PIN_PULL(NONE),
	S3C64XX_GPH1_MMC1_CMD, S3C_PIN_PULL(NONE),
	S3C64XX_GPH2_MMC1_DATA0, S3C_PIN_PULL(NONE),
	S3C64XX_GPH3_MMC1_DATA1, S3C_PIN_PULL(NONE),
	S3C64XX_GPH4_MMC1_DATA2, S3C_PIN_PULL(NONE),
	S3C64XX_GPH5_MMC1_DATA3, S3C_PIN_PULL(NONE),
	/* LCD */
	S3C64XX_GPI0_LCD_VD0, S3C_PIN_PULL(NONE),
	S3C64XX_GPI1_LCD_VD1, S3C_PIN_PULL(NONE),
	S3C64XX_GPI2_LCD_VD2, S3C_PIN_PULL(NONE),
	S3C64XX_GPI3_LCD_VD3, S3C_PIN_PULL(NONE),
	S3C64XX_GPI4_LCD_VD4, S3C_PIN_PULL(NONE),
	S3C64XX_GPI5_LCD_VD5, S3C_PIN_PULL(NONE),
	S3C64XX_GPI6_LCD_VD6, S3C_PIN_PULL(NONE),
	S3C64XX_GPI7_LCD_VD7, S3C_PIN_PULL(NONE),
	S3C64XX_GPI8_LCD_VD8, S3C_PIN_PULL(NONE),
	S3C64XX_GPI9_LCD_VD9, S3C_PIN_PULL(NONE),
	S3C64XX_GPI10_LCD_VD10, S3C_PIN_PULL(NONE),
	S3C64XX_GPI11_LCD_VD11, S3C_PIN_PULL(NONE),
	S3C64XX_GPI12_LCD_VD12, S3C_PIN_PULL(NONE),
	S3C64XX_GPI13_LCD_VD13, S3C_PIN_PULL(NONE),
	S3C64XX_GPI14_LCD_VD14, S3C_PIN_PULL(NONE),
	S3C64XX_GPI15_LCD_VD15, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ0_LCD_VD16, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ1_LCD_VD17, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ2_LCD_VD18, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ3_LCD_VD19, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ4_LCD_VD20, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ5_LCD_VD21, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ6_LCD_VD22, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ7_LCD_VD23, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ8_LCD_HSYNC, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ9_LCD_VSYNC, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ10_LCD_VDEN, S3C_PIN_PULL(NONE),
	S3C64XX_GPJ11_LCD_VCLK, S3C_PIN_PULL(NONE),
	/* Keypad */
	S3C64XX_GPK8_KEYPAD_ROW0, S3C_PIN_PULL(NONE), //KB
	S3C64XX_GPK9_KEYPAD_ROW1, S3C_PIN_PULL(NONE), //KB
	S3C64XX_GPK10_KEYPAD_ROW2, S3C_PIN_PULL(NONE), //KB
	S3C64XX_GPL0_KEYPAD_COL0, S3C_PIN_PULL(NONE),
	S3C64XX_GPL1_KEYPAD_COL1, S3C_PIN_PULL(NONE),
	S3C64XX_GPL2_KEYPAD_COL2, S3C_PIN_PULL(NONE),
	/* OneNAND */
	S3C64XX_GPO0_MEM0_NCS2, S3C_PIN_PULL(NONE),

	/* Inputs */
	S3C_PIN(GPIO_PS_VOUT_30), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BOOT_MODE), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_LCD_ID), 		S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_REV1), 		S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_REV2), 		S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_REV3), 		S3C_PIN_IN, S3C_PIN_PULL(NONE),
	/* Outputs */
	S3C_PIN(GPIO_BT_RST_N), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BT_WLAN_REG_ON), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_WLAN_RST_N), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_WLAN_WAKE), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_MCAM_RST_N), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_TFLASH_EN), 	S3C_PIN_CFG(1), S3C_PIN_PULL(NONE),
	//S3C_PIN(GPIO_TFLASH_EN(1)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_CAM_EN), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_EARPATH_SEL), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_EAR_CP_CODEC_SW), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_MAIN_CP_CODEC_SW), S3C_PIN_OUT(1), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_FM_RST), 		S3C_PIN_OUT(1), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_CAM_VGA_RST_N), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_CAM_VGA_STBY_N), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_MSENSE_RST), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_USIM_BOOT), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_5M_EN), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_VIB_EN), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_USB_SEL), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_TV_EN), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_TOUCH_EN), 	S3C_PIN_OUT(1), S3C_PIN_PULL(UP),
	S3C_PIN(GPIO_EXTWKUP), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BT_WAKE), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_ALPS_ON), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_MAX8906_AMP_EN), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PDA_ACTIVE), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_FM_LDO_ON), 	S3C_PIN_OUT(1), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PHONE_ON), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_CP_BOOT_SEL), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PCM_SEL), 		S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_LCD_CS_N), 	S3C_PIN_CFG(1), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_LCD_SDI), 		S3C_PIN_CFG(1), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_LCD_RST_N), 	S3C_PIN_CFG(1), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_LCD_SCLK), 	S3C_PIN_CFG(1), S3C_PIN_PULL(NONE),
	//S3C_PIN(GPIO_PDA_PS_HOLD), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	//S3C_PIN(GPIO_UART_SEL), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_VT_CP_SW), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	/* Bit banged I2C */
	S3C_PIN(GPIO_USBSW_SCL_3V0), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_USBSW_SDA_3V0), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_FM_I2C_SCL), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_FM_I2C_SDA), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PWR_I2C_SCL), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PWR_I2C_SDA),	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_AP_SCL_1_8V), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_AP_SDA_1_8V), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),

	/* EINTs */
//	S3C_PIN(GPIO_HOLD_KEY_N), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_TA_SEL), 		S3C_PIN_IN, S3C_PIN_PULL(UP),
//	S3C_PIN(GPIO_TOUCH_INT_N), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BT_HOST_WAKE), 	S3C_PIN_IN, S3C_PIN_PULL(DOWN),
//	S3C_PIN(GPIO_TA_CHG_N), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_ONEDRAM_INT_N), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_WLAN_HOST_WAKE), 	S3C_PIN_IN, S3C_PIN_PULL(DOWN),
	S3C_PIN(GPIO_MSENSE_INT), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_ACC_INT), 		S3C_PIN_IN, S3C_PIN_PULL(NONE),
//	S3C_PIN(GPIO_SIM_DETECT_N), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_POWER_N), 		S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_TF_DETECT), 	S3C_PIN_CFG(3), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PHONE_ACTIVE), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PMIC_INT_N), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_JACK_INT_N), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_DET_35), 		S3C_PIN_CFG(3), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_FM_INT), 		S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_EAR_SEND_END), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_RESOUT_N), 	S3C_PIN_IN, S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BOOT_EINT13), 	S3C_PIN_CFG(1), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BOOT_EINT14), 	S3C_PIN_CFG(1), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BOOT_EINT15), 	S3C_PIN_CFG(1), S3C_PIN_PULL(NONE),
	/* Unused pins */
	S3C_PIN(S3C64XX_GPA(0)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPA(1)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPA(2)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPA(3)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPB(4)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPC(0)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPC(6)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPC(7)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPD(1)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPF(13)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPF(14)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPO(1)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPO(4)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPO(13)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
//	S3C_PIN(S3C64XX_GPP(8)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),	// CHECK
//	S3C_PIN(S3C64XX_GPP(10)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),	// CHECK
//	S3C_PIN(S3C64XX_GPP(14)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),	// CHECK
	S3C_PIN(S3C64XX_GPQ(0)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPQ(1)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPQ(3)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPQ(6)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
	S3C_PIN(S3C64XX_GPQ(7)), 	S3C_PIN_OUT(0), S3C_PIN_PULL(NONE),
};

static struct s3c_pin_cfg_entry spica_slp_config[] __initdata = {
	/* UART 1 (Bluetooth) */
	S3C64XX_PIN(GPA(4)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* RXD */
	S3C64XX_PIN(GPA(5)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* TXD */
	S3C64XX_PIN(GPA(6)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* CTSn */
	S3C64XX_PIN(GPA(7)), S3C64XX_PIN_SLP(HIGH), S3C_PIN_PULL(NONE), /* RTSn */

	/* UART 2 (External / USB SW) */
	//S3C64XX_PIN(GPB(0)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* RXD */
	//S3C64XX_PIN(GPB(1)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* TXD */

	/* UART 3 (Phone) */
	S3C64XX_PIN(GPB(2)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* RXD */
	S3C64XX_PIN(GPB(3)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* TXD */

	/* I2C 0 */
	S3C64XX_PIN(GPB(5)), S3C64XX_PIN_SLP(HIGH), S3C_PIN_PULL(NONE), /* SCL */
	S3C64XX_PIN(GPB(6)), S3C64XX_PIN_SLP(HIGH), S3C_PIN_PULL(NONE), /* SDA */

	/* MMC 2 (WLAN) */
	S3C64XX_PIN(GPC(4)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* CMD */
	S3C64XX_PIN(GPC(5)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* CLK */
	S3C64XX_PIN(GPH(6)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA0 */
	S3C64XX_PIN(GPH(7)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA1 */
	S3C64XX_PIN(GPH(8)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA2 */
	S3C64XX_PIN(GPH(9)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA3 */

	/* I2S 0 */
	S3C64XX_PIN(GPD(0)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* CLK */
	S3C64XX_PIN(GPD(2)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* LRCLK */
	S3C64XX_PIN(GPD(3)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* DI */
	S3C64XX_PIN(GPD(4)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* DO */

	/* PCM 1 */
	S3C64XX_PIN(GPE(0)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* SCLK */
	S3C64XX_PIN(GPE(2)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* FSYNC */
	S3C64XX_PIN(GPE(3)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* SIN */
	S3C64XX_PIN(GPE(4)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* SOUT */

	/* CAMIF */
	S3C64XX_PIN(GPF(0)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* CLK */
	S3C64XX_PIN(GPF(1)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* HREF */
	S3C64XX_PIN(GPF(2)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* PCLK */
	S3C64XX_PIN(GPF(4)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* VSYNC */
	S3C64XX_PIN(GPF(5)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* YDATA0 */
	S3C64XX_PIN(GPF(6)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* YDATA1 */
	S3C64XX_PIN(GPF(7)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* YDATA2 */
	S3C64XX_PIN(GPF(8)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* YDATA3 */
	S3C64XX_PIN(GPF(9)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* YDATA4 */
	S3C64XX_PIN(GPF(10)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* YDATA5 */
	S3C64XX_PIN(GPF(11)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* YDATA6 */
	S3C64XX_PIN(GPF(12)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* YDATA7 */

	/* PWM */
	S3C64XX_PIN(GPF(15)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* TOUT1 */

	/* MMC 0 (TF) */
	S3C64XX_PIN(GPG(0)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* CLK */
	S3C64XX_PIN(GPG(1)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* CMD */
	S3C64XX_PIN(GPG(2)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA0 */
	S3C64XX_PIN(GPG(3)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA1 */
	S3C64XX_PIN(GPG(4)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA2 */
	S3C64XX_PIN(GPG(5)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA3 */

	/* MMC 1 (iNAND) */
	S3C64XX_PIN(GPH(0)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* CLK */
	S3C64XX_PIN(GPH(1)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* CMD */
	S3C64XX_PIN(GPH(2)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA0 */
	S3C64XX_PIN(GPH(3)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA1 */
	S3C64XX_PIN(GPH(4)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA2 */
	S3C64XX_PIN(GPH(5)), S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), /* DATA3 */

	/* LCD */
	S3C64XX_PIN(GPI(0)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD0 */
	S3C64XX_PIN(GPI(1)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD1 */
	S3C64XX_PIN(GPI(2)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD2 */
	S3C64XX_PIN(GPI(3)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD3 */
	S3C64XX_PIN(GPI(4)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD4 */
	S3C64XX_PIN(GPI(5)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD5 */
	S3C64XX_PIN(GPI(6)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD6 */
	S3C64XX_PIN(GPI(7)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD7 */
	S3C64XX_PIN(GPI(8)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD8 */
	S3C64XX_PIN(GPI(9)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD9 */
	S3C64XX_PIN(GPI(10)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD10 */
	S3C64XX_PIN(GPI(11)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD11 */
	S3C64XX_PIN(GPI(12)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD12 */
	S3C64XX_PIN(GPI(13)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD13 */
	S3C64XX_PIN(GPI(14)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD14 */
	S3C64XX_PIN(GPI(15)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD15 */
	S3C64XX_PIN(GPJ(0)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD16 */
	S3C64XX_PIN(GPJ(1)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD17 */
	S3C64XX_PIN(GPJ(2)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD18 */
	S3C64XX_PIN(GPJ(3)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD19 */
	S3C64XX_PIN(GPJ(4)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD20 */
	S3C64XX_PIN(GPJ(5)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD21 */
	S3C64XX_PIN(GPJ(6)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD22 */
	S3C64XX_PIN(GPJ(7)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VD23 */
	S3C64XX_PIN(GPJ(8)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* HSYNC */
	S3C64XX_PIN(GPJ(9)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VSYNC */
	S3C64XX_PIN(GPJ(10)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VDEN */
	S3C64XX_PIN(GPJ(11)), S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE), /* VCLK */

	/* Keypad */
	S3C_PIN(GPIO_KEYSCAN_0), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),		//C
	S3C_PIN(GPIO_KEYSCAN_1), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),		//C
	S3C_PIN(GPIO_KEYSCAN_2),	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),		//C

	/* OneNAND */
	S3C_PIN(GPIO_MEM0_NCS2),	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN), /* nCS2 */

	/* Inputs */
	S3C_PIN(GPIO_PS_VOUT_30), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BOOT_MODE), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_LCD_ID), 		S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE),	//C orginal mach-jet.c has GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE
	S3C_PIN(GPIO_REV1), 		S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_REV2), 		S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_REV3), 		S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE),

	/* Outputs */
	S3C_PIN(GPIO_BT_RST_N), 	S3C64XX_PIN_SLP(RETAIN), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BT_WLAN_REG_ON), 	S3C64XX_PIN_SLP(RETAIN), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_WLAN_RST_N), 	S3C64XX_PIN_SLP(RETAIN), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_WLAN_WAKE), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_MCAM_RST_N), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_TFLASH_EN),	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_CAM_EN),		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	//C
	S3C_PIN(GPIO_EARPATH_SEL),	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_EAR_CP_CODEC_SW),	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_MAIN_CP_CODEC_SW),	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_FM_RST),		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_CAM_VGA_RST_N), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_CAM_VGA_STBY_N), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_MSENSE_RST), 	S3C64XX_PIN_SLP(HIGH), S3C_PIN_PULL(NONE),	//C
	S3C_PIN(GPIO_USIM_BOOT), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	//C
	S3C_PIN(GPIO_5M_EN),		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_VIB_EN), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	//C
	S3C_PIN(GPIO_USB_SEL), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_TV_EN), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_TOUCH_EN), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	//C
	S3C_PIN(GPIO_EXTWKUP), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_BT_WAKE), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_ALPS_ON), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_MAX8906_AMP_EN),	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PDA_ACTIVE), 	S3C64XX_PIN_SLP(RETAIN), S3C_PIN_PULL(NONE),	//C //no SLP in original mach-jet.c 
	S3C_PIN(GPIO_FM_LDO_ON), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(DOWN),	//C
	S3C_PIN(GPIO_PHONE_ON), 	S3C64XX_PIN_SLP(RETAIN), S3C_PIN_PULL(NONE),	//C //no SLP in original mach-jet.c 
	S3C_PIN(GPIO_CP_BOOT_SEL), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_PCM_SEL), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_LCD_CS_N), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	//C orginal mach-jet.c has GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE
	S3C_PIN(GPIO_LCD_SDI), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	//C orginal mach-jet.c has GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE
	S3C_PIN(GPIO_LCD_RST_N), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	//C orginal mach-jet.c has GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE
	S3C_PIN(GPIO_LCD_SCLK), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	//C orginal mach-jet.c has GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE
	S3C_PIN(GPIO_PDA_PS_HOLD), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	//S3C_PIN(GPIO_UART_SEL), 	S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C_PIN(GPIO_VT_CP_SW), 	S3C64XX_PIN_SLP(RETAIN), S3C_PIN_PULL(NONE),

	/* Bit banged I2C */
//	S3C_PIN(GPIO_USBSW_SCL_3V0), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), //C //no SLP in original mach-jet.c 
//	S3C_PIN(GPIO_USBSW_SDA_3V0), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), //C //no SLP in original mach-jet.c 
//	S3C_PIN(GPIO_FM_I2C_SCL), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), //C //no SLP in original mach-jet.c 
//	S3C_PIN(GPIO_FM_I2C_SDA), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), //C //no SLP in original mach-jet.c 
//	S3C_PIN(GPIO_PWR_I2C_SCL), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), //C //no SLP in original mach-jet.c 
//	S3C_PIN(GPIO_PWR_I2C_SDA), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), //C //no SLP in original mach-jet.c 
//	S3C_PIN(GPIO_AP_SCL_1_8V), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), //C //no SLP in original mach-jet.c 
//	S3C_PIN(GPIO_AP_SDA_1_8V), 	S3C64XX_PIN_SLP(IN), S3C_PIN_PULL(NONE), //C //no SLP in original mach-jet.c 

	/* Unused pins */
	S3C64XX_PIN(GPA(0)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPA(1)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPA(2)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPA(3)),		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPB(4)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPC(0)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPC(6)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPC(7)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPD(1)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPF(13)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPF(14)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPO(1)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPO(4)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPO(13)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
//	S3C64XX_PIN(GPP(8)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	// CHECK
//	S3C64XX_PIN(GPP(10)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	// CHECK
//	S3C64XX_PIN(GPP(14)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),	// CHECK
	S3C64XX_PIN(GPQ(0)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPQ(1)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPQ(3)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPQ(6)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
	S3C64XX_PIN(GPQ(7)), 		S3C64XX_PIN_SLP(LOW), S3C_PIN_PULL(NONE),
};

/*
 * Machine setup
 */

static void __init spica_fixup(struct machine_desc *desc,
		struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;

	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = PHYS_SIZE;
}

static void __init spica_map_io(void)
{
#if defined(CONFIG_SPICA_AHB_166) || defined(CONFIG_SPICA_CPU_667_AHB_166)
	u32 reg;
#endif
	s3c64xx_init_io(spica_iodesc, ARRAY_SIZE(spica_iodesc));
#if defined(CONFIG_SPICA_AHB_166)
	reg = __raw_readl(S3C64XX_OTHERS);
	reg &= ~S3C64XX_OTHERS_SYNCMODE;
	reg &= ~S3C64XX_OTHERS_SYNCMUXSEL;
	__raw_writel(reg, S3C64XX_OTHERS);

	while (__raw_readl(S3C64XX_OTHERS) & S3C64XX_OTHERS_SYNCACK_MASK);

	reg = __raw_readl(S3C_CLK_DIV0);
	reg &= ~S3C6400_CLKDIV0_HCLK2_MASK;
	reg |= 0x0 << S3C6400_CLKDIV0_HCLK2_SHIFT;
	__raw_writel(reg, S3C_CLK_DIV0);

	__raw_writel(0xc14d0302, S3C_MPLL_CON);
#elif defined(CONFIG_SPICA_CPU_667_AHB_166)
	reg = __raw_readl(S3C64XX_OTHERS);
	reg &= ~S3C64XX_OTHERS_SYNCMODE;
	reg &= ~S3C64XX_OTHERS_SYNCMUXSEL;
	__raw_writel(reg, S3C64XX_OTHERS);

	while (__raw_readl(S3C64XX_OTHERS) & S3C64XX_OTHERS_SYNCACK_MASK);

	__raw_writel(0xc14d0301, S3C_APLL_CON);

	reg = __raw_readl(S3C_CLK_DIV0);
	reg &= ~S3C6400_CLKDIV0_HCLK2_MASK;
	reg |= 0x1 << S3C6400_CLKDIV0_HCLK2_SHIFT;
	__raw_writel(reg, S3C_CLK_DIV0);

	reg = __raw_readl(S3C64XX_OTHERS);
	reg |= S3C64XX_OTHERS_SYNCMODE;
	reg |= S3C64XX_OTHERS_SYNCMUXSEL;
	__raw_writel(reg, S3C64XX_OTHERS);

	do {
		reg = __raw_readl(S3C64XX_OTHERS);
		reg &= S3C64XX_OTHERS_SYNCACK_MASK;
	} while (reg != S3C64XX_OTHERS_SYNCACK_MASK);
#endif
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(spica_uartcfgs, ARRAY_SIZE(spica_uartcfgs)-1);
}

static void spica_poweroff(void)
{
	gpio_direction_output(GPIO_PDA_PS_HOLD, 0);

	while(1);
}

static void __init spica_machine_init(void)
{
	struct clk *parent;
	struct clk *clk;
	unsigned long rate;

	/* Setup frequencies of some clocks */
	clk = clk_get(NULL, "hclk");
	rate = clk_get_rate(clk);
	clk_put(clk);

	parent = clk_get(NULL, "dout_mpll");
	clk = clk_get(NULL, "uclk1");
	clk_set_rate(parent, rate);
	clk_set_parent(clk, parent);
	clk_set_rate(clk, rate);
	clk_put(clk);
	clk_put(parent);

	clk = clk_get(NULL, "mfc_sclk");
	clk_set_rate(clk, rate);
	clk_put(clk);

	/* Misc tweaks */
	__raw_writel(0x7702, S3C64XX_QOS_OVERRIDE1);
	__raw_writel(0x3ffff, S3C64XX_MISC_CON);

	/* Setup interrupt filtering */
	__raw_writel(0x88888888, S3C64XX_EINT0FLTCON0);
	__raw_writel(0x88888888, S3C64XX_EINT0FLTCON1);
	__raw_writel(0x88888888, S3C64XX_EINT0FLTCON2);
	__raw_writel(0x00008888, S3C64XX_EINT0FLTCON3);
	__raw_writel(0x00848484, S3C64XX_EINT12FLTCON);
	__raw_writel(0x84848484, S3C64XX_EINT34FLTCON);
	__raw_writel(0x84848484, S3C64XX_EINT56FLTCON);
	__raw_writel(0x84848484, S3C64XX_EINT78FLTCON);
	__raw_writel(0x00000084, S3C64XX_EINT9FLTCON);

	/* Setup sleep mode settings */
	__raw_writel(0x1020, S3C64XX_SPCONSLP);
	__raw_writel(0x00005000, S3C64XX_MEM0CONSLP0);
	__raw_writel(0x01041595, S3C64XX_MEM0CONSLP1);
	__raw_writel(0x10055000, S3C64XX_MEM1CONSLP);
	__raw_writel(__raw_readl(S3C64XX_SLEEP_CFG) & ~0x61, S3C64XX_SLEEP_CFG);

	/* Configure GPIO pins */
	s3c_pin_config(spica_pin_config, ARRAY_SIZE(spica_pin_config));
	s3c_pin_slp_config(spica_slp_config, ARRAY_SIZE(spica_slp_config));

	/* Setup Bluetooth and WLAN */
	spica_bt_lpm_init();
	gpio_request(GPIO_BT_WLAN_REG_ON, "WLAN/BT power");
	gpio_request(GPIO_WLAN_RST_N, "WLAN reset");
	gpio_request(GPIO_BT_RST_N, "BT reset");
//	gpio_request(GPIO_MICBIAS_EN, "MIC bias");

	/* Setup power management */
	gpio_request(GPIO_PDA_PS_HOLD, "Power hold");
	pm_power_off = spica_poweroff;
	s3c_pm_init();

	/* Register I2C devices */
	s3c_i2c0_set_platdata(&jet_sensor_i2c);
	i2c_register_board_info(jet_sensor_i2c.bus_num, jet_sensor_i2c_devs,
					ARRAY_SIZE(jet_sensor_i2c_devs));
	i2c_register_board_info(jet_pmic_i2c.id, jet_pmic_i2c_devs,
					ARRAY_SIZE(jet_pmic_i2c_devs));
	i2c_register_board_info(jet_usbsw_i2c.id, jet_usbsw_i2c_devs,
					ARRAY_SIZE(jet_usbsw_i2c_devs));
	i2c_register_board_info(jet_fm_i2c.id, jet_fm_i2c_devs,
					ARRAY_SIZE(jet_fm_i2c_devs));
	i2c_register_board_info(jet_audio_i2c.id, jet_audio_i2c_devs,
					ARRAY_SIZE(jet_audio_i2c_devs));

	/* Setup framebuffer */
	s3c_fb_set_platdata(&jet_lcd_pdata);

	/* Setup SDHCI */
	s3c_sdhci0_set_platdata(&jet_hsmmc0_pdata);
	s3c_sdhci0_set_platdata(&jet_hsmmc1_pdata);
	s3c_sdhci2_set_platdata(&spica_hsmmc2_pdata);

	/* Setup keypad */
//	samsung_keypad_set_platdata(&spica_keypad_pdata);
	samsung_keypad_set_platdata(&jet_keypad_pdata);

	/* Setup touchscreen */
	s3c24xx_ts_set_platdata(&s3c_ts_platform);

	/* Setup OneNAND */
	s3c_set_platdata(&spica_onenand_pdata, sizeof(spica_onenand_pdata),
							&s3c_device_onenand);

	/* Setup audio */
	s3c_set_platdata(&spica_i2s_pdata, sizeof(spica_i2s_pdata),
							&s3c64xx_device_iis0);

	/* Setup power domains */
	s3c_device_fb.dev.parent = &s3c64xx_device_pd[S3C64XX_DOMAIN_F].dev;
	samsung_pd_set_persistent(&s3c64xx_device_pd[S3C64XX_DOMAIN_F]);
	s3c64xx_add_pd_devices();

	/* Wakelock for WLAN sleep workaround */
	wake_lock_init(&wlan_wakelock, WAKE_LOCK_SUSPEND, "wlan");
	/* Setup RAM console */
	spica_ram_console_resources[0].start =
		allocate_contiguous_memory_nomap(RAM_CONSOLE_SIZE,
								0, PAGE_SIZE);
	spica_ram_console_resources[0].end = RAM_CONSOLE_SIZE +
				spica_ram_console_resources[0].start - 1;

	/* Register platform devices */
	platform_add_devices(spica_devices, ARRAY_SIZE(spica_devices));
	platform_add_devices(spica_mod_devices, ARRAY_SIZE(spica_mod_devices));

	/* Register PMEM devices */
	spica_add_pmem_devices();

	/* Indicate full regulator constraints */
	regulator_has_full_constraints();

	/* For telephony modules */
	sec_class = class_create(THIS_MODULE, "sec");
	WARN_ON(IS_ERR(sec_class));
}

/*
 * Machine definition
 */

MACHINE_START(GT_I5700, "Jet")
	/* Maintainer: Tomasz Figa <tomasz.figa at gmail.com> */
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,
	.init_irq	= s3c6410_init_irq,
	.fixup		= spica_fixup,
	.map_io		= spica_map_io,
	.reserve	= spica_reserve,
	.init_machine	= spica_machine_init,
	.timer		= &s3c64xx_timer,
MACHINE_END
