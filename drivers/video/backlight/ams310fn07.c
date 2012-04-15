/*
 * drivers/video/backlight/s6d05a.c
 *
 * Copyright (C) 2011 Tomasz Figa <tomasz.figa at gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/leds.h>

#include <linux/i2c/maximi2c.h>

#include <plat/gpio-cfg.h>

#include <mach/hardware.h>

#include <linux/wait.h>
#include <linux/spi/spi.h>
#include <linux/backlight.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>

#include <linux/miscdevice.h>

#include <mach/gpio.h>

#include <video/ams310fn07.h>

#define BACKLIGHT_STATUS_ALC	0x100
#define BACKLIGHT_LEVEL_VALUE	0x0FF	/* 0 ~ 255 */

#define BACKLIGHT_LEVEL_MIN		0
#define BACKLIGHT_LEVEL_MAX		BACKLIGHT_LEVEL_VALUE

#define BACKLIGHT_LEVEL_DEFAULT	100		/* Default Setting */

#define ON 	1
#define OFF	0

#define AMS310fN07_SPI_DELAY_USECS	5

/*
 * Driver data
 */
struct ams310fn07_data {
	struct backlight_device	*bl;
	struct device *dev;

	unsigned cs_gpio;
	unsigned sck_gpio;
	unsigned sda_gpio;
	unsigned reset_gpio;

	struct regulator *vdd3;
	struct regulator *vci;

	int state;
	int brightness;

	struct setting_table *standby_off;
	int standby_off_size;
	struct setting_table *power_on;
	int power_on_size;
	struct setting_table *power_off;
	int power_off_size;
	struct setting_table *display_on;
	int display_on_size;
	struct setting_table *display_off;
	int display_off_size;
	struct setting_table (*gamma_setting_idl)[GAMMA_SETTINGS];
	struct setting_table (*gamma_setting_vid)[GAMMA_SETTINGS];
	struct setting_table (*gamma_setting_cam)[GAMMA_SETTINGS];
	int gamma_setting_size;
};

struct ams310fn07_data *g_data;
int lcd_power = OFF;
EXPORT_SYMBOL(lcd_power);

void ams310fn07_set_power(struct ams310fn07_data *data, int power);
EXPORT_SYMBOL(ams310fn07_set_power);

int backlight_power = OFF;
EXPORT_SYMBOL(backlight_power);

void backlight_power_ctrl(struct ams310fn07_data *data, s32 value);
EXPORT_SYMBOL(backlight_power_ctrl);

int backlight_level = BACKLIGHT_LEVEL_DEFAULT;
EXPORT_SYMBOL(backlight_level);

void backlight_level_ctrl(struct ams310fn07_data *data, s32 value);
EXPORT_SYMBOL(backlight_level_ctrl);

static struct setting_table standby_off_table[] = {
   	//{ 0x0300 ,  0 }, original
   	{ 0x1DA0 , 0},
   	{ 0x1403 ,  0 },
};

#define STANDBY_OFF	(int)(sizeof(standby_off_table)/sizeof(struct setting_table))


static struct setting_table power_on_setting_table[] = {
/* power setting sequence + init */
    { 0x3108 ,   0 }, //HCLK default
    { 0x3214 ,   0 }, //20 HCLK default
    { 0x3002 ,   0 },
    { 0x2703 ,   0 },
    { 0x1208 ,   0 }, //VBP 8
    { 0x1308 ,   0 }, //VFP 8
    { 0x1510 ,   0 }, //VFP 8
    { 0x1600 ,   0 }, //RGB sync set 00= 24 bit 01=16 bit
    { 0xEFD0 ,   0 }, //pentile key? or E8
    //{ 0x72E8 ,   0 }, //isnt right yet!
    //{ 0x3944 ,   0 }, //gamma set select
};


#define POWER_ON_SETTINGS	(int)(sizeof(power_on_setting_table)/sizeof(struct setting_table))

static struct setting_table display_on_setting_table[] = {
        { 0x1722 ,   0 },//boosting freq
        { 0x1833 ,   0 }, //amp set
        { 0x1903 ,   0 }, //gamma amp
        { 0x1A01 ,   0 }, //VLOUT1: 2x VLOUT2:3x VLOUT3=4x
        { 0x22A4 ,   0 }, //VCC
        { 0x2300 ,   0 }, //VCL
        { 0x26A0 ,   0 }, //DOTCLK REFERENCE
   	{ 0x1DA0 ,  100 },
   	{ 0x1403 ,  0 },
};

#define DISPLAY_ON_SETTINGS	(int)(sizeof(display_on_setting_table)/sizeof(struct setting_table))

static struct setting_table display_off_setting_table[] = {
    { 0x1400, 0 },
    { 0x1DA1, 0 },
};

#define DISPLAY_OFF_SETTINGS	(int)(sizeof(display_off_setting_table)/sizeof(struct setting_table))

static struct setting_table power_off_setting_table[] = {
    { 0x1400, 0 },
    { 0x1DA1,  10 },
};


#define POWER_OFF_SETTINGS	(int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))

static struct setting_table gamma_setting_table[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	// set 1.1
		{ 0x4000,	0},
		{ 0x4100,	0 },
		{ 0x4207,	0 },
		{ 0x4310,	0 },
		{ 0x4411,	0 },
		{ 0x450C,	0 },
		{ 0x4626,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5310,	0 },
		{ 0x5410,	0 },
		{ 0x550C,	0 },
		{ 0x5626,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6204,	0 },
		{ 0x630F,	0 },
		{ 0x6410,	0 },
		{ 0x650A,	0 },
		{ 0x6634,	0 },
	},
	{	// set 1.2
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x4209,	0 },
		{ 0x4315,	0 },
		{ 0x4416,	0 },
		{ 0x4511,	0 },
		{ 0x4632,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5315,	0 },
		{ 0x5416,	0 },
		{ 0x5511,	0 },
		{ 0x5633,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6206,	0 },
		{ 0x6314,	0 },
		{ 0x6415,	0 },
		{ 0x650e,	0 },
		{ 0x6646,	0 },
	},
	{	// set 1.3
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420a,	0 },
		{ 0x4317,	0 },
		{ 0x4418,	0 },
		{ 0x4512,	0 },
		{ 0x4636,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5317,	0 },
		{ 0x5417,	0 },
		{ 0x5512,	0 },
		{ 0x5636,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6206,	0 },
		{ 0x6316,	0 },
		{ 0x6416,	0 },
		{ 0x650f,	0 },
		{ 0x664b,	0 },
	},
	{	// set 1.4
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420a,	0 },
		{ 0x4318,	0 },
		{ 0x4419,	0 },
		{ 0x4513,	0 },
		{ 0x4639,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5318,	0 },
		{ 0x5419,	0 },
		{ 0x5513,	0 },
		{ 0x5639,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6317,	0 },
		{ 0x6417,	0 },
		{ 0x650f,	0 },
		{ 0x664f,	0 },
	},
	{	// set 1.5
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420b,	0 },
		{ 0x4319,	0 },
		{ 0x441a,	0 },
		{ 0x4513,	0 },
		{ 0x463b,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5319,	0 },
		{ 0x541a,	0 },
		{ 0x5513,	0 },
		{ 0x563b,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6318,	0 },
		{ 0x6418,	0 },
		{ 0x6510,	0 },
		{ 0x6652,	0 },
	},
	{	// set 1.6
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420b,	0 },
		{ 0x431a,	0 },
		{ 0x441c,	0 },
		{ 0x4514,	0 },
		{ 0x463e,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531a,	0 },
		{ 0x541b,	0 },
		{ 0x5514,	0 },
		{ 0x563e,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6319,	0 },
		{ 0x641A,	0 },
		{ 0x6511,	0 },
		{ 0x6656,	0 },
	},
	{	// set 1.7
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420c,	0 },
		{ 0x431b,	0 },
		{ 0x441d,	0 },
		{ 0x4515,	0 },
		{ 0x4641,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531b,	0 },
		{ 0x541c,	0 },
		{ 0x5515,	0 },
		{ 0x5641,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631a,	0 },
		{ 0x641b,	0 },
		{ 0x6512,	0 },
		{ 0x665a,	0 },
	},
	{	// set 1.8
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420c,	0 },
		{ 0x431d,	0 },
		{ 0x441e,	0 },
		{ 0x4516,	0 },
		{ 0x4643,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531d,	0 },
		{ 0x541e,	0 },
		{ 0x5516,	0 },
		{ 0x5644,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631b,	0 },
		{ 0x641c,	0 },
		{ 0x6512,	0 },
		{ 0x665e,	0 },
	},
	{	// set 1.9
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x431e,	0 },
		{ 0x441f,	0 },
		{ 0x4517,	0 },
		{ 0x4646,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531e,	0 },
		{ 0x541e,	0 },
		{ 0x5517,	0 },
		{ 0x5646,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631c,	0 },
		{ 0x641d,	0 },
		{ 0x6513,	0 },
		{ 0x6661,	0 },
	},
	{	// set 1.10
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x431f,	0 },
		{ 0x4420,	0 },
		{ 0x4518,	0 },
		{ 0x4648,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531f,	0 },
		{ 0x541f,	0 },
		{ 0x5518,	0 },
		{ 0x5649,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631d,	0 },
		{ 0x641e,	0 },
		{ 0x6514,	0 },
		{ 0x6664,	0 },
	},
	{	// set 1.11
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x4320,	0 },
		{ 0x4422,	0 },
		{ 0x4519,	0 },
		{ 0x464b,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5320,	0 },
		{ 0x5421,	0 },
		{ 0x5519,	0 },
		{ 0x564c,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631e,	0 },
		{ 0x641f,	0 },
		{ 0x6514,	0 },
		{ 0x6668,	0 },
	},
	{	// set 1.12
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420e,	0 },
		{ 0x4320,	0 },
		{ 0x4422,	0 },
		{ 0x4519,	0 },
		{ 0x464c,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5320,	0 },
		{ 0x5421,	0 },
		{ 0x5519,	0 },
		{ 0x564d,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631f,	0 },
		{ 0x6420,	0 },
		{ 0x6515,	0 },
		{ 0x6669,	0 },
	},
	{	// set 1.13
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420e,	0 },
		{ 0x4321,	0 },
		{ 0x4423,	0 },
		{ 0x451a,	0 },
		{ 0x464e,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5321,	0 },
		{ 0x5422,	0 },
		{ 0x551a,	0 },
		{ 0x564f,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6320,	0 },
		{ 0x6420,	0 },
		{ 0x6515,	0 },
		{ 0x666d,	0 },
	},
	{	// set 1.14
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420f,	0 },
		{ 0x4322,	0 },
		{ 0x4424,	0 },
		{ 0x451b,	0 },
		{ 0x4650,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5322,	0 },
		{ 0x5423,	0 },
		{ 0x551b,	0 },
		{ 0x5651,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6320,	0 },
		{ 0x6421,	0 },
		{ 0x6516,	0 },
		{ 0x6670,	0 },
	},
	{	// set 1.15
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420f,	0 },
		{ 0x4323,	0 },
		{ 0x4425,	0 },
		{ 0x451c,	0 },
		{ 0x4653,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5323,	0 },
		{ 0x5424,	0 },
		{ 0x551c,	0 },
		{ 0x5654,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6321,	0 },
		{ 0x6422,	0 },
		{ 0x6517,	0 },
		{ 0x6673,	0 },
	},
	{	// set 1.16
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x4210,	0 },
		{ 0x4325,	0 },
		{ 0x4427,	0 },
		{ 0x451D,	0 },
		{ 0x4656,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5325,	0 },
		{ 0x5426,	0 },
		{ 0x551D,	0 },
		{ 0x5657,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620B,	0 },
		{ 0x6323,	0 },
		{ 0x6424,	0 },
		{ 0x6518,	0 },
		{ 0x6677,	0 },
	},
};

static struct setting_table gamma_setting_table_video[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	// set 2.1
		{ 0x4000,	0},
		{ 0x4100,	0 },
		{ 0x4207,	0 },
		{ 0x4310,	0 },
		{ 0x4411,	0 },
		{ 0x450C,	0 },
		{ 0x4626,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5310,	0 },
		{ 0x5410,	0 },
		{ 0x550C,	0 },
		{ 0x5626,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6204,	0 },
		{ 0x630F,	0 },
		{ 0x6410,	0 },
		{ 0x650A,	0 },
		{ 0x6634,	0 },
	},
	{	// set 2.2
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x4209,	0 },
		{ 0x4315,	0 },
		{ 0x4416,	0 },
		{ 0x4511,	0 },
		{ 0x4632,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5315,	0 },
		{ 0x5416,	0 },
		{ 0x5511,	0 },
		{ 0x5633,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6206,	0 },
		{ 0x6314,	0 },
		{ 0x6415,	0 },
		{ 0x650e,	0 },
		{ 0x6646,	0 },
	},
	{	// set 2.3
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420a,	0 },
		{ 0x4317,	0 },
		{ 0x4418,	0 },
		{ 0x4512,	0 },
		{ 0x4636,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5317,	0 },
		{ 0x5417,	0 },
		{ 0x5512,	0 },
		{ 0x5636,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6206,	0 },
		{ 0x6316,	0 },
		{ 0x6416,	0 },
		{ 0x650f,	0 },
		{ 0x664b,	0 },
	},
	{	// set 2.4
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420a,	0 },
		{ 0x4318,	0 },
		{ 0x4419,	0 },
		{ 0x4513,	0 },
		{ 0x4639,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5318,	0 },
		{ 0x5419,	0 },
		{ 0x5513,	0 },
		{ 0x5639,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6317,	0 },
		{ 0x6417,	0 },
		{ 0x650f,	0 },
		{ 0x664f,	0 },
	},
	{	// set 2.5
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420b,	0 },
		{ 0x4319,	0 },
		{ 0x441a,	0 },
		{ 0x4513,	0 },
		{ 0x463b,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5319,	0 },
		{ 0x541a,	0 },
		{ 0x5513,	0 },
		{ 0x563b,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6318,	0 },
		{ 0x6418,	0 },
		{ 0x6510,	0 },
		{ 0x6652,	0 },
	},
	{	// set 1.6
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420b,	0 },
		{ 0x431a,	0 },
		{ 0x441c,	0 },
		{ 0x4514,	0 },
		{ 0x463e,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531a,	0 },
		{ 0x541b,	0 },
		{ 0x5514,	0 },
		{ 0x563e,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6319,	0 },
		{ 0x641A,	0 },
		{ 0x6511,	0 },
		{ 0x6656,	0 },
	},
	{	// set 2.7
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420c,	0 },
		{ 0x431b,	0 },
		{ 0x441d,	0 },
		{ 0x4515,	0 },
		{ 0x4641,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531b,	0 },
		{ 0x541c,	0 },
		{ 0x5515,	0 },
		{ 0x5641,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631a,	0 },
		{ 0x641b,	0 },
		{ 0x6512,	0 },
		{ 0x665a,	0 },
	},
	{	// set 2.8
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420c,	0 },
		{ 0x431d,	0 },
		{ 0x441e,	0 },
		{ 0x4516,	0 },
		{ 0x4643,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531d,	0 },
		{ 0x541e,	0 },
		{ 0x5516,	0 },
		{ 0x5644,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631b,	0 },
		{ 0x641c,	0 },
		{ 0x6512,	0 },
		{ 0x665e,	0 },
	},
	{	// set 2.9
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x431e,	0 },
		{ 0x441f,	0 },
		{ 0x4517,	0 },
		{ 0x4646,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531e,	0 },
		{ 0x541e,	0 },
		{ 0x5517,	0 },
		{ 0x5646,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631c,	0 },
		{ 0x641d,	0 },
		{ 0x6513,	0 },
		{ 0x6661,	0 },
	},
	{	// set 2.10
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x431f,	0 },
		{ 0x4420,	0 },
		{ 0x4518,	0 },
		{ 0x4648,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531f,	0 },
		{ 0x541f,	0 },
		{ 0x5518,	0 },
		{ 0x5649,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631d,	0 },
		{ 0x641e,	0 },
		{ 0x6514,	0 },
		{ 0x6664,	0 },
	},
	{	// set 2.11
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x4320,	0 },
		{ 0x4422,	0 },
		{ 0x4519,	0 },
		{ 0x464b,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5320,	0 },
		{ 0x5421,	0 },
		{ 0x5519,	0 },
		{ 0x564c,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631e,	0 },
		{ 0x641f,	0 },
		{ 0x6514,	0 },
		{ 0x6668,	0 },
	},
	{	// set 2.12
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420e,	0 },
		{ 0x4320,	0 },
		{ 0x4422,	0 },
		{ 0x4519,	0 },
		{ 0x464c,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5320,	0 },
		{ 0x5421,	0 },
		{ 0x5519,	0 },
		{ 0x564d,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631f,	0 },
		{ 0x6420,	0 },
		{ 0x6515,	0 },
		{ 0x6669,	0 },
	},
	{	// set 2.13
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420e,	0 },
		{ 0x4321,	0 },
		{ 0x4423,	0 },
		{ 0x451a,	0 },
		{ 0x464e,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5321,	0 },
		{ 0x5422,	0 },
		{ 0x551a,	0 },
		{ 0x564f,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6320,	0 },
		{ 0x6420,	0 },
		{ 0x6515,	0 },
		{ 0x666d,	0 },
	},
	{	// set 2.14
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420f,	0 },
		{ 0x4322,	0 },
		{ 0x4424,	0 },
		{ 0x451b,	0 },
		{ 0x4650,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5322,	0 },
		{ 0x5423,	0 },
		{ 0x551b,	0 },
		{ 0x5651,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6320,	0 },
		{ 0x6421,	0 },
		{ 0x6516,	0 },
		{ 0x6670,	0 },
	},
	{	// set 2.15
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420f,	0 },
		{ 0x4323,	0 },
		{ 0x4425,	0 },
		{ 0x451c,	0 },
		{ 0x4653,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5323,	0 },
		{ 0x5424,	0 },
		{ 0x551c,	0 },
		{ 0x5654,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6321,	0 },
		{ 0x6422,	0 },
		{ 0x6517,	0 },
		{ 0x6673,	0 },
	},
	{	// set 2.16
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x4210,	0 },
		{ 0x4325,	0 },
		{ 0x4427,	0 },
		{ 0x451D,	0 },
		{ 0x4656,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5325,	0 },
		{ 0x5426,	0 },
		{ 0x551D,	0 },
		{ 0x5657,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620B,	0 },
		{ 0x6323,	0 },
		{ 0x6424,	0 },
		{ 0x6518,	0 },
		{ 0x6677,	0 },
	},
};


static struct setting_table gamma_setting_table_cam[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	// set 3.1
		{ 0x4000,	0},
		{ 0x4100,	0 },
		{ 0x4207,	0 },
		{ 0x4310,	0 },
		{ 0x4411,	0 },
		{ 0x450C,	0 },
		{ 0x4626,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5310,	0 },
		{ 0x5410,	0 },
		{ 0x550C,	0 },
		{ 0x5626,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6204,	0 },
		{ 0x630F,	0 },
		{ 0x6410,	0 },
		{ 0x650A,	0 },
		{ 0x6634,	0 },
	},
	{	// set 3.2
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x4209,	0 },
		{ 0x4315,	0 },
		{ 0x4416,	0 },
		{ 0x4511,	0 },
		{ 0x4632,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5315,	0 },
		{ 0x5416,	0 },
		{ 0x5511,	0 },
		{ 0x5633,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6206,	0 },
		{ 0x6314,	0 },
		{ 0x6415,	0 },
		{ 0x650e,	0 },
		{ 0x6646,	0 },
	},
	{	// set 3.3
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420a,	0 },
		{ 0x4317,	0 },
		{ 0x4418,	0 },
		{ 0x4512,	0 },
		{ 0x4636,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5317,	0 },
		{ 0x5417,	0 },
		{ 0x5512,	0 },
		{ 0x5636,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6206,	0 },
		{ 0x6316,	0 },
		{ 0x6416,	0 },
		{ 0x650f,	0 },
		{ 0x664b,	0 },
	},
	{	// set 3.4
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420a,	0 },
		{ 0x4318,	0 },
		{ 0x4419,	0 },
		{ 0x4513,	0 },
		{ 0x4639,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5318,	0 },
		{ 0x5419,	0 },
		{ 0x5513,	0 },
		{ 0x5639,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6317,	0 },
		{ 0x6417,	0 },
		{ 0x650f,	0 },
		{ 0x664f,	0 },
	},
	{	// set 3.5
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420b,	0 },
		{ 0x4319,	0 },
		{ 0x441a,	0 },
		{ 0x4513,	0 },
		{ 0x463b,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5319,	0 },
		{ 0x541a,	0 },
		{ 0x5513,	0 },
		{ 0x563b,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6318,	0 },
		{ 0x6418,	0 },
		{ 0x6510,	0 },
		{ 0x6652,	0 },
	},
	{	// set 3.6
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420b,	0 },
		{ 0x431a,	0 },
		{ 0x441c,	0 },
		{ 0x4514,	0 },
		{ 0x463e,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531a,	0 },
		{ 0x541b,	0 },
		{ 0x5514,	0 },
		{ 0x563e,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6207,	0 },
		{ 0x6319,	0 },
		{ 0x641A,	0 },
		{ 0x6511,	0 },
		{ 0x6656,	0 },
	},
	{	// set 3.7
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420c,	0 },
		{ 0x431b,	0 },
		{ 0x441d,	0 },
		{ 0x4515,	0 },
		{ 0x4641,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531b,	0 },
		{ 0x541c,	0 },
		{ 0x5515,	0 },
		{ 0x5641,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631a,	0 },
		{ 0x641b,	0 },
		{ 0x6512,	0 },
		{ 0x665a,	0 },
	},
	{	// set 3.8
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420c,	0 },
		{ 0x431d,	0 },
		{ 0x441e,	0 },
		{ 0x4516,	0 },
		{ 0x4643,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531d,	0 },
		{ 0x541e,	0 },
		{ 0x5516,	0 },
		{ 0x5644,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631b,	0 },
		{ 0x641c,	0 },
		{ 0x6512,	0 },
		{ 0x665e,	0 },
	},
	{	// set 3.9
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x431e,	0 },
		{ 0x441f,	0 },
		{ 0x4517,	0 },
		{ 0x4646,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531e,	0 },
		{ 0x541e,	0 },
		{ 0x5517,	0 },
		{ 0x5646,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6208,	0 },
		{ 0x631c,	0 },
		{ 0x641d,	0 },
		{ 0x6513,	0 },
		{ 0x6661,	0 },
	},
	{	// set 3.10
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x431f,	0 },
		{ 0x4420,	0 },
		{ 0x4518,	0 },
		{ 0x4648,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x531f,	0 },
		{ 0x541f,	0 },
		{ 0x5518,	0 },
		{ 0x5649,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631d,	0 },
		{ 0x641e,	0 },
		{ 0x6514,	0 },
		{ 0x6664,	0 },
	},
	{	// set 3.11
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420d,	0 },
		{ 0x4320,	0 },
		{ 0x4422,	0 },
		{ 0x4519,	0 },
		{ 0x464b,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5320,	0 },
		{ 0x5421,	0 },
		{ 0x5519,	0 },
		{ 0x564c,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631e,	0 },
		{ 0x641f,	0 },
		{ 0x6514,	0 },
		{ 0x6668,	0 },
	},
	{	// set 3.12
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420e,	0 },
		{ 0x4320,	0 },
		{ 0x4422,	0 },
		{ 0x4519,	0 },
		{ 0x464c,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5320,	0 },
		{ 0x5421,	0 },
		{ 0x5519,	0 },
		{ 0x564d,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x6209,	0 },
		{ 0x631f,	0 },
		{ 0x6420,	0 },
		{ 0x6515,	0 },
		{ 0x6669,	0 },
	},
	{	// set 3.13
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420e,	0 },
		{ 0x4321,	0 },
		{ 0x4423,	0 },
		{ 0x451a,	0 },
		{ 0x464e,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5321,	0 },
		{ 0x5422,	0 },
		{ 0x551a,	0 },
		{ 0x564f,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6320,	0 },
		{ 0x6420,	0 },
		{ 0x6515,	0 },
		{ 0x666d,	0 },
	},
	{	// set 3.14
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420f,	0 },
		{ 0x4322,	0 },
		{ 0x4424,	0 },
		{ 0x451b,	0 },
		{ 0x4650,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5322,	0 },
		{ 0x5423,	0 },
		{ 0x551b,	0 },
		{ 0x5651,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6320,	0 },
		{ 0x6421,	0 },
		{ 0x6516,	0 },
		{ 0x6670,	0 },
	},
	{	// set 3.15
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x420f,	0 },
		{ 0x4323,	0 },
		{ 0x4425,	0 },
		{ 0x451c,	0 },
		{ 0x4653,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5323,	0 },
		{ 0x5424,	0 },
		{ 0x551c,	0 },
		{ 0x5654,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620a,	0 },
		{ 0x6321,	0 },
		{ 0x6422,	0 },
		{ 0x6517,	0 },
		{ 0x6673,	0 },
	},
	{	// set 3.16
		{ 0x4000,	0 },
		{ 0x4100,	0 },
		{ 0x4210,	0 },
		{ 0x4325,	0 },
		{ 0x4427,	0 },
		{ 0x451D,	0 },
		{ 0x4656,	0 },
		{ 0x5000,	0 },
		{ 0x5100,	0 },
		{ 0x5200,	0 },
		{ 0x5325,	0 },
		{ 0x5426,	0 },
		{ 0x551D,	0 },
		{ 0x5657,	0 },
		{ 0x6000,	0 },
		{ 0x6100,	0 },
		{ 0x620B,	0 },
		{ 0x6323,	0 },
		{ 0x6424,	0 },
		{ 0x6518,	0 },
		{ 0x6677,	0 },
	},
};

/*
 * Hardware interface
 */
static inline void ams310fn07_send_word(struct ams310fn07_data *data, u16 word)
{
	u8 ID, ID2,word_hi, word_lo, mask;
	word_hi = (word >> 8); // last byte
	word_lo = word; //firt byte
	ID = 0x70;
	ID2 = 0x72;

	mask = 1 << 7;

	gpio_set_value(data->sck_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 0);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->sck_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	do {
		gpio_set_value(data->sck_gpio, 0);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sda_gpio, ID & mask);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sck_gpio, 1);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		mask >>= 1;
	} while (mask);

	mask = 1 << 7;

	do {
		gpio_set_value(data->sck_gpio, 0);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sda_gpio, word_hi & mask);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sck_gpio, 1);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		mask >>= 1;
	} while (mask);

	gpio_set_value(data->sck_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->sda_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->sck_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 0);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	mask = 1 << 7;

	do {
		gpio_set_value(data->sck_gpio, 0);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sda_gpio, ID2 & mask);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sck_gpio, 1);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		mask >>= 1;
	} while (mask);

	mask = 1 << 7;

	do {
		gpio_set_value(data->sck_gpio, 0);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sda_gpio, word_lo & mask);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sck_gpio, 1);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		mask >>= 1;
	} while (mask);

	gpio_set_value(data->sck_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->sda_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

}

static inline void ams310fn07_send_word2(struct ams310fn07_data *data, u8 word)
{
	u8 ID2, mask;
	ID2 = 0x72;

	mask = 1 << 7;

	gpio_set_value(data->sck_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 0);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->sck_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	do {
		gpio_set_value(data->sck_gpio, 0);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sda_gpio, ID2 & mask);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sck_gpio, 1);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		mask >>= 1;
	} while (mask);

	mask = 1 << 7;

	do {
		gpio_set_value(data->sck_gpio, 0);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sda_gpio, word & mask);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		gpio_set_value(data->sck_gpio, 1);
		udelay(AMS310fN07_SPI_DELAY_USECS);

		mask >>= 1;
	} while (mask);

	gpio_set_value(data->sck_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->sda_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

	gpio_set_value(data->cs_gpio, 1);
	udelay(AMS310fN07_SPI_DELAY_USECS);

}

static void ams310fn07_send_command_seq(struct ams310fn07_data *data, struct setting_table *table, int size)
{
	int i;
	for (i = 0; i < size; i++, table++) {
			ams310fn07_send_word(data, table->reg_data);
			if(table->wait)
				msleep(table->wait);
	}
}

/*
 *	LCD Power Handler
 */

#define MAX8698_ID		0xCC

#define ONOFF2			0x01

#define ONOFF2_ELDO6	(0x01 << 7)
#define ONOFF2_ELDO7	(0x03 << 6)
static s32 old_level = 0;

typedef enum {
	LCD_IDLE = 0,
	LCD_VIDEO,
	LCD_CAMERA
} lcd_gamma_status;

int lcd_gamma_present = 0;

void lcd_gamma_change(struct ams310fn07_data *data, int gamma_status)
{
	if(old_level < 1){
		printk("OLD level <1: %d \n", old_level);
		return;
	}
		
	//printk("[S3C LCD] %s : level %d, gamma_status %d\n", __FUNCTION__, (old_level-1), gamma_status);

	if(gamma_status == LCD_IDLE)
	{
		lcd_gamma_present = LCD_IDLE;
		ams310fn07_send_command_seq(data, data->gamma_setting_idl[1], data->gamma_setting_size);
	}
	else if(gamma_status == LCD_VIDEO)
	{
		lcd_gamma_present = LCD_VIDEO;
		ams310fn07_send_command_seq(data, data->gamma_setting_idl[1], data->gamma_setting_size);
	}
	else if(gamma_status == LCD_CAMERA)
	{
		lcd_gamma_present = LCD_CAMERA;
		ams310fn07_send_command_seq(data, data->gamma_setting_idl[1], data->gamma_setting_size);
	}
	else
		return;
}

EXPORT_SYMBOL(lcd_gamma_change);

/*
 * Backlight interface
 */
void ams310fn07_set_power(struct ams310fn07_data *data, int power)
{

	//printk(" LCD power ctrl called \n" );

	if (data->state == power)
		return;

	if (power) {
		//printk("Lcd power on sequence start\n");
		pm_runtime_get_sync(data->dev);

		gpio_set_value(data->reset_gpio, 0);

		msleep(20);

		gpio_set_value(data->reset_gpio, 1);

		msleep(20);

		ams310fn07_send_command_seq(data, data->power_on, data->power_on_size);

		ams310fn07_send_word2(data, 0xE8);

		switch(lcd_gamma_present)
		{
			printk("[S3C LCD] %s : level dimming, lcd_gamma_present %d\n", __FUNCTION__, lcd_gamma_present);
			ams310fn07_send_word(data, 0x3944);

			case LCD_IDLE:
				ams310fn07_send_command_seq(data, data->gamma_setting_idl[1], data->gamma_setting_size);
				break;
			case LCD_VIDEO:
				ams310fn07_send_command_seq(data, data->gamma_setting_vid[1], data->gamma_setting_size);
				break;
			case LCD_CAMERA:
				ams310fn07_send_command_seq(data, data->gamma_setting_cam[1], data->gamma_setting_size);
				break;
			default:
				break;
		}

		ams310fn07_send_command_seq(data, data->display_on, data->display_on_size);
		//printk("Lcd power on sequence end\n");


	} else {

		//printk("Lcd power off sequence start\n");
		/* Power Off Sequence */
		ams310fn07_send_command_seq(data, data->display_off, data->display_off_size);

		gpio_set_value(data->reset_gpio, 0);

		gpio_set_value(data->cs_gpio, 0);
		gpio_set_value(data->sck_gpio, 0);

		pm_runtime_put_sync(data->dev);

		//printk("Lcd power off sequence end\n");
		//printk("XXXXXXXXXXXXXXXXXXXXXXXXXXX\n");

	}

	data->state = power;
}

static void ams310fn07_set_backlight(struct ams310fn07_data *data, s32 value)
{
	s32 level;
	//printk("%s is called !! \n", __func__);

	value &= BACKLIGHT_LEVEL_VALUE;

		if (value == 0)
			level = 0;
		else if ((value > 0) && (value < 15))
			level = 1;
		else	
			level = (((value - 15) / 15)); 
	if (level) {	
	//printk(" backlight_ctrl : level:%x, old_level: %x \n", level, old_level);
		if (level != old_level) {
			old_level = level;

			if (lcd_power == OFF)
			{
				ams310fn07_set_power(data, ON);
			}

			//printk("LCD Backlight level setting value ==> %d  , level ==> %d \n",value,level);
			
			switch(lcd_gamma_present)
			{
				printk("[S3C LCD] %s : level %d, lcd_gamma_present %d\n", __FUNCTION__, (level-1), lcd_gamma_present);

				case LCD_IDLE:
					ams310fn07_send_command_seq(data, data->gamma_setting_idl[(level-1)], data->gamma_setting_size);
					break;
				case LCD_VIDEO:
					ams310fn07_send_command_seq(data, data->gamma_setting_vid[(level-1)], data->gamma_setting_size);
					break;
				case LCD_CAMERA:
					ams310fn07_send_command_seq(data, data->gamma_setting_cam[(level-1)], data->gamma_setting_size);
					break;
				default:
					break;
			}

		}
	}
	else {
		old_level = level;
		ams310fn07_set_power(data, OFF);
	}
}

void backlight_level_ctrl(struct ams310fn07_data *data, s32 value)
{
	if ((value < BACKLIGHT_LEVEL_MIN) ||	/* Invalid Value */
		(value > BACKLIGHT_LEVEL_MAX) ||
		(value == data->brightness))	/* Same Value */
		return;

	if (backlight_power)
		ams310fn07_set_backlight(data, value);
	
	data->brightness = value;
}

void backlight_power_ctrl(struct ams310fn07_data *data, s32 value)
{
	if ((value < OFF) ||	/* Invalid Value */
		(value > ON))
		return;

	ams310fn07_set_backlight(data, (value ? backlight_level : OFF));
	
	backlight_power = (value ? ON : OFF);
}

static int ams310fn07_bl_update_status(struct backlight_device *bl)
{
	struct ams310fn07_data *data = bl_get_data(bl);
	int new_state = 1;

	if (!bl->props.brightness || bl->props.power != FB_BLANK_UNBLANK
					|| bl->props.state & BL_CORE_FBBLANK
					|| bl->props.state & BL_CORE_SUSPENDED)
		new_state = 0;

	data->brightness = bl->props.brightness;

	ams310fn07_set_power(data, new_state);

	if (new_state)
		ams310fn07_set_backlight(data, data->brightness);

	return 0;
}

static int ams310fn07_bl_get_brightness(struct backlight_device *bl)
{
	struct ams310fn07_data *data = bl_get_data(bl);

	if (!data->state)
		return 0;

	return data->brightness;
}

static int ams310fn07_bl_check_fb(struct backlight_device *bl, struct fb_info *info)
{
	struct ams310fn07_data *data = bl_get_data(bl);

	if (data->dev->parent == NULL)
		return 1;

	return data->dev->parent == info->device;
}

static struct backlight_ops ams310fn07_bl_ops = {
	.options	= BL_CORE_SUSPENDRESUME,
	.update_status	= ams310fn07_bl_update_status,
	.get_brightness	= ams310fn07_bl_get_brightness,
	.check_fb	= ams310fn07_bl_check_fb,
};

/*
 * Platform driver
 */
static int __devinit ams310fn07_probe(struct platform_device *pdev)
{
	struct ams310fn07_data *data;
	struct ams310fn07_platform_data *pdata = pdev->dev.platform_data;
	int ret = 0;

	if (!pdata)
		return -ENOENT;

	ret = gpio_request(pdata->reset_gpio, "ams310fn07 reset");
	if (ret)
		return ret;

	ret = gpio_direction_output(pdata->reset_gpio, 0);
	if (ret)
		goto err_reset;

	ret = gpio_request(pdata->cs_gpio, "ams310fn07 cs");
	if (ret)
		goto err_reset;

	ret = gpio_direction_output(pdata->cs_gpio, 0);
	if (ret)
		goto err_cs;

	ret = gpio_request(pdata->sck_gpio, "ams310fn07 sck");
	if (ret)
		goto err_cs;

	ret = gpio_direction_output(pdata->sck_gpio, 0);
	if (ret)
		goto err_sck;

	ret = gpio_request(pdata->sda_gpio, "ams310fn07 sda");
	if (ret)
		goto err_sck;

	ret = gpio_direction_output(pdata->sda_gpio, 0);
	if (ret)
		goto err_sda;

	data = kzalloc(sizeof(struct ams310fn07_data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&pdev->dev, "No memory for device state\n");
		ret = -ENOMEM;
		goto err_sda;
	}

	g_data = data;

	data->dev = &pdev->dev;
	data->reset_gpio = pdata->reset_gpio;
	data->cs_gpio = pdata->cs_gpio;
	data->sck_gpio = pdata->sck_gpio;
	data->sda_gpio = pdata->sda_gpio;

	if (pdata->standby_off)
		data->standby_off = pdata->standby_off;
	else
		data->standby_off = standby_off_table;
	data->standby_off_size = (int)(sizeof(standby_off_table)/sizeof(struct setting_table));

	if (pdata->power_on)
		data->power_on = pdata->power_on;
	else
		data->power_on = power_on_setting_table;
	data->power_on_size = (int)(sizeof(power_on_setting_table)/sizeof(struct setting_table));

	if (pdata->power_off)
		data->power_off = pdata->power_off;
	else
		data->power_off = power_off_setting_table;
	data->power_off_size = (int)(sizeof(power_off_setting_table)/sizeof(struct setting_table));

	if (pdata->display_on)
		data->display_on = pdata->display_on;
	else
		data->display_on = display_on_setting_table;
	data->display_on_size = (int)(sizeof(display_on_setting_table)/sizeof(struct setting_table));

	if (pdata->display_off)
		data->display_off = pdata->display_off;
	else
		data->display_off = display_off_setting_table;
	data->display_off_size = (int)(sizeof(display_off_setting_table)/sizeof(struct setting_table));

	if (pdata->gamma_setting_idl)
		data->gamma_setting_idl = pdata->gamma_setting_idl;
	else
		data->gamma_setting_idl = gamma_setting_table;

	if (pdata->gamma_setting_vid)
		data->gamma_setting_vid = pdata->gamma_setting_vid;
	else
		data->gamma_setting_vid = gamma_setting_table_video;

	if (pdata->gamma_setting_cam)
		data->gamma_setting_cam = pdata->gamma_setting_cam;
	else
		data->gamma_setting_cam = gamma_setting_table_cam;
	data->gamma_setting_size = GAMMA_SETTINGS;

/*	data->vci = regulator_get(&pdev->dev, "vci");
	if (IS_ERR(data->vci)) {
		dev_err(&pdev->dev, "Failed to get vci regulator\n");
		ret = PTR_ERR(data->vci);
		goto err_free;
	}

	data->vdd3 = regulator_get(&pdev->dev, "vdd3");
	if (IS_ERR(data->vdd3)) {
		dev_err(&pdev->dev, "Failed to get vdd3 regulator\n");
		ret = PTR_ERR(data->vdd3);
		goto err_vci;
	}
*/
	platform_set_drvdata(pdev, data);

	pm_runtime_enable(data->dev);
	pm_runtime_no_callbacks(data->dev);

	data->bl = backlight_device_register(dev_driver_string(&pdev->dev),
			&pdev->dev, data, &ams310fn07_bl_ops, NULL);
	if (IS_ERR(data->bl)) {
		dev_err(&pdev->dev, "Failed to register backlight device\n");
		ret = PTR_ERR(data->bl);
		goto err_vdd3;
	}

	data->bl->props.max_brightness	= 255;
	data->bl->props.brightness	= 128;
	data->bl->props.type		= BACKLIGHT_RAW;
	data->bl->props.power		= FB_BLANK_UNBLANK;
	backlight_update_status(data->bl);

	data->state = ON;

	backlight_level_ctrl(data, BACKLIGHT_LEVEL_DEFAULT);

	backlight_power = ON;

	return 0;

err_vdd3:
//	regulator_put(data->vdd3);
//err_vci:
//	regulator_put(data->vci);
//err_free:
	kfree(data);
err_sda:
	gpio_free(pdata->sda_gpio);
err_sck:
	gpio_free(pdata->sck_gpio);
err_cs:
	gpio_free(pdata->cs_gpio);
err_reset:
	gpio_free(pdata->reset_gpio);

	return ret;
}

static int __devexit ams310fn07_remove(struct platform_device *pdev)
{
	struct ams310fn07_data *data = platform_get_drvdata(pdev);

	ams310fn07_set_power(data, 0);

	backlight_device_unregister(data->bl);

	gpio_free(data->reset_gpio);
	gpio_free(data->cs_gpio);
	gpio_free(data->sck_gpio);
	gpio_free(data->sda_gpio);

	regulator_put(data->vci);
	regulator_put(data->vdd3);

	kfree(data);

	return 0;
}

static int ams310fn07_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ams310fn07_data *data = platform_get_drvdata(pdev);

	ams310fn07_set_power(data, 0);

	return 0;
}

static int ams310fn07_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ams310fn07_data *data = platform_get_drvdata(pdev);

	ams310fn07_bl_update_status(data->bl);

	return 0;
}

static struct dev_pm_ops ams310fn07_pm_ops = {
	.suspend	= ams310fn07_suspend,
	.resume		= ams310fn07_resume,
};

static void ams310fn07_shutdown(struct platform_device *pdev)
{
	struct ams310fn07_data *data = platform_get_drvdata(pdev);

	ams310fn07_set_power(data, 0);
}

static struct platform_driver ams310fn07_driver = {
	.driver = {
		.name	= "ams310fn07-lcd",
		.owner	= THIS_MODULE,
		.pm	= &ams310fn07_pm_ops,
	},
	.probe		= ams310fn07_probe,
	.remove		= ams310fn07_remove,
	.shutdown	= ams310fn07_shutdown,
};

/*
 * Module
 */
static int __init ams310fn07_init(void)
{
	return platform_driver_register(&ams310fn07_driver);
}
module_init(ams310fn07_init);

static void __exit ams310fn07_exit(void)
{
	platform_driver_unregister(&ams310fn07_driver);
}
module_exit(ams310fn07_exit);
