/*
 * max8906.c - mfd core driver for the Maxim 8906
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
 * This driver is based on max8906.c and max8906.c by 
 *  Maxim Firmware Group (C) 2004 Maxim Integrated Products
 *  MyungJoo Ham <myungjoo.ham@samsung.com>
 */

#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/pm_runtime.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max8906.h>
#include <linux/mfd/max8906-private.h>

#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX "MAX8906: "
//#define PMIC_EXTRA_DEBUG

#define MAX8906_RTC_ID	0xD0	/* Read Time Clock */
#define MAX8906_ADC_ID	0x8E	/* ADC/Touchscreen */
#define MAX8906_GPM_ID	0x78	/* General Power Management */
#define MAX8906_APM_ID	0x68	/* APP Specific Power Management */

#define I2C_ADDR_GPM	(MAX8906_GPM_ID >> 1)
#define I2C_ADDR_APM	(MAX8906_APM_ID >> 1)
#define I2C_ADDR_ADC	(MAX8906_ADC_ID >> 1)
#define I2C_ADDR_RTC	(MAX8906_RTC_ID >> 1)

static struct mfd_cell max8906_devs[] = {
	{ .name = "max8906-pmic", },
//	{ .name = "max8906-rtc", },
//	{ .name = "max8906-battery", },
};

static struct i2c_driver max8906_i2c_driver;

static struct i2c_client *max8906_rtc_i2c_client = NULL;
static struct i2c_client *max8906_adc_i2c_client = NULL;
static struct i2c_client *max8906_gpm_i2c_client = NULL;
static struct i2c_client *max8906_apm_i2c_client = NULL;


/*
 * new I2C Interface
 */

static int max8906_i2c_device_read(struct max8906_data *max8906, u8 reg, u8 *dest)
{
	struct i2c_client *client = max8906->i2c_client;
	int ret;

	mutex_lock(&max8906->lock);

	ret = i2c_smbus_read_byte_data(client, reg);

	mutex_unlock(&max8906->lock);

	if (ret < 0)
		return ret;

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL(max8906_i2c_device_read);

static int max8906_i2c_device_update(struct max8906_data *max8906, u8 reg,
				     u8 val, u8 mask)
{
	struct i2c_client *client = max8906->i2c_client;
	int ret;

	mutex_lock(&max8906->lock);

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(client, reg, new_val);
		if (ret >= 0)
			ret = 0;
	}

	mutex_unlock(&max8906->lock);

	return ret;
}
EXPORT_SYMBOL(max8906_i2c_device_update);


int max8906_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max8906_dev *max8906 = i2c_get_clientdata(i2c);
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = i2c->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = i2c->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	//mutex_lock(&max8906->iolock);
	//ret = i2c_smbus_read_byte_data(i2c, reg);
	//mutex_unlock(&max8906->iolock);
	//if (ret < 0)
	//	return ret;
	//ret &= 0xff;
	//*dest = ret;

	mutex_lock(&max8906->iolock);
	ret = i2c_transfer(i2c->adapter, msg, 2);
	mutex_unlock(&max8906->iolock);
	if (ret != 2) 
		return -EIO;

	*dest = buf[0];
	return 0;
}
EXPORT_SYMBOL(max8906_read_reg);

int max8906_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max8906_dev *max8906 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max8906->iolock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max8906->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(max8906_bulk_read);

int max8906_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max8906_dev *max8906 = i2c_get_clientdata(i2c);
	int ret=0;
	u8 buf[2];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = value;

	msg[0].addr = i2c->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	mutex_lock(&max8906->iolock);
	ret = i2c_transfer(i2c->adapter, msg, 1);
	mutex_unlock(&max8906->iolock);
	if (ret != 1) 
		return -EIO;

	if ((i2c->addr == MAX8906_GPM_ID) && (reg>0x83)&&(reg < 0x8f)) {
	  pr_info(PREFIX "%s:I: addr 0x%02X reg 0x%02X Succeeded!\n", __func__, i2c->addr, reg);
	}

//	mutex_lock(&max8906->iolock);
//	ret = i2c_smbus_write_byte_data(i2c, reg, value);
//	mutex_unlock(&max8906->iolock);
	return ret;
}
EXPORT_SYMBOL(max8906_write_reg);

int max8906_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max8906_dev *max8906 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max8906->iolock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max8906->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(max8906_bulk_write);

int max8906_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max8906_dev *max8906 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max8906->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max8906->iolock);
	return ret;
}
EXPORT_SYMBOL(max8906_update_reg);

/*
 * old I2C Interface
 */

unsigned int pmic_read(u8 slaveaddr, u8 reg, u8 *data, u8 length)
{
	struct i2c_client *client;
#ifdef PMIC_EXTRA_DEBUG	
	printk("%s -> slaveaddr 0x%02x, reg 0x%02x, data 0x%02x\n",	__FUNCTION__, slaveaddr, reg, *data);
#endif	
	if (slaveaddr == MAX8906_GPM_ID)
		client = max8906_gpm_i2c_client;
	else if (slaveaddr == MAX8906_APM_ID)
		client = max8906_apm_i2c_client;
	else if (slaveaddr == MAX8906_ADC_ID)
		client = max8906_adc_i2c_client;
	else if (slaveaddr == MAX8906_RTC_ID)
		client = max8906_rtc_i2c_client;
	else 
		return PMIC_FAIL;

	if (max8906_read_reg(client, reg, data) < 0) { 
#ifdef PMIC_EXTRA_DEBUG	
		printk(KERN_ERR "%s -> Failed! (slaveaddr 0x%02x, reg 0x%02x, data 0x%02x)\n",
					__FUNCTION__, slaveaddr, reg, *data);
#endif
		return PMIC_FAIL;
	}	

	return PMIC_PASS;
}

unsigned int pmic_write(u8 slaveaddr, u8 reg, u8 *data, u8 length)
{
	struct i2c_client *client;
#ifdef PMIC_EXTRA_DEBUG	
	printk("%s -> slaveaddr 0x%02x, reg 0x%02x, data 0x%02x\n",	__FUNCTION__, slaveaddr, reg, *data);
#endif	
	if (slaveaddr == MAX8906_GPM_ID)
		client = max8906_gpm_i2c_client;
	else if (slaveaddr == MAX8906_APM_ID)
		client = max8906_apm_i2c_client;
	else if (slaveaddr == MAX8906_ADC_ID)
		client = max8906_adc_i2c_client;
	else if (slaveaddr == MAX8906_RTC_ID)
		client = max8906_rtc_i2c_client;
	else 
		return PMIC_FAIL;

	if (max8906_write_reg(client, reg, *data) < 0) { 
		printk(KERN_ERR "%s -> Failed! (slaveaddr 0x%02x, reg 0x%02x, data 0x%02x)\n",
					__FUNCTION__, slaveaddr, reg, *data);
		return PMIC_FAIL;
	}	

	return PMIC_PASS;
}

/*
 * debug
 */

void max8906_debug_print( void)
{
  u8 sec,min,hour,day,month,yearm,yearl;
    boolean status;
    byte tscbuff;
    byte result[2];
    int voltage;

  pmic_read( MAX8906_RTC_ID, 0x00, &sec, 1);
  pmic_read( MAX8906_RTC_ID, 0x01, &min, 1);
  pmic_read( MAX8906_RTC_ID, 0x02, &hour, 1);
  pmic_read( MAX8906_RTC_ID, 0x04, &day, 1);
  pmic_read( MAX8906_RTC_ID, 0x05, &month, 1);
  pmic_read( MAX8906_RTC_ID, 0x06, &yearl, 1);
  pmic_read( MAX8906_RTC_ID, 0x07, &yearm, 1);
  printk("[MAX8906]: DATE,TIME: %X.%X.%X%X, %X:%02X:%02X\n", day,month,yearm,yearl,hour,min,sec);

  // here we read some couple of registers
  /* pmic_read( MAX8906_ADC_ID, 0x0, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_STA_INT=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x1, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_INT_MSK=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x2, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_CNFG1  =0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x3, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_CNFG2  =0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x4, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_CNFG3  =0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x5, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_CNFG4  =0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x6, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_RES_CF1=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x7, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_AVG_CF1=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x8, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_ACQ_CF1=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x9, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_ACQ_CF2=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0xA, &tscbuff, 1); printk("[MAX8906 dbg]: TSC_ACQ_CF3=0x%02X\n",tscbuff); */

  /* // set MBATT measurement */
  /* if (FALSE == Set_MAX8906_TSC_CONV_REG(MBATT_Measurement, CONT)) { */
  /*   printk("[MAX8906] failed to start MBATT Measurement\n"); */
  /*   return; */
  /* } */
  /* tscbuff = 0x5; // Vref & CONT */
  /* pmic_write( MAX8906_ADC_ID, 0xD8, &tscbuff, 1); printk("[MAX8906 dbg]: wrtite 0xD8=0x%02X\n",tscbuff); */


  /* pmic_read( MAX8906_ADC_ID, 0x60, &tscbuff, 1); printk("[MAX8906 dbg]: 0x60=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x61, &tscbuff, 1); printk("[MAX8906 dbg]: 0x61=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x62, &tscbuff, 1); printk("[MAX8906 dbg]: 0x62=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x63, &tscbuff, 1); printk("[MAX8906 dbg]: 0x63=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x64, &tscbuff, 1); printk("[MAX8906 dbg]: 0x64=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x65, &tscbuff, 1); printk("[MAX8906 dbg]: 0x65=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x66, &tscbuff, 1); printk("[MAX8906 dbg]: 0x66=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x67, &tscbuff, 1); printk("[MAX8906 dbg]: 0x67=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x68, &tscbuff, 1); printk("[MAX8906 dbg]: 0x68=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x69, &tscbuff, 1); printk("[MAX8906 dbg]: 0x69=0x%02X\n",tscbuff); */
  /* pmic_read( MAX8906_ADC_ID, 0x6A, &tscbuff, 1); printk("[MAX8906 dbg]: 0x6a=0x%02X\n",tscbuff); */

    //  Measurement done?
  /* do { */
  /*   status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff); */
  /* } while ( !(status && (tsc_buff & nCONV_NS_M)) ); */

  /*   // set BBATT measurement */
  /* Set_MAX8906_TSC_CONV_REG(BBATT_Measurement, CONT); */

  /*   //  Measurement done? */
  /* do { */
  /*   status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff); */
  /* } while ( !(status && (tsc_buff & nCONV_NS_M)) ); */

  /*   // set VBUS measurement */
  /* Set_MAX8906_TSC_CONV_REG(VBUS_Measurement, CONT); */

  /*   //  Measurement done? */
  /* do { */
  /*   status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff); */
  /* } while ( !(status && (tsc_buff & nCONV_NS_M)) ); */

  /* // GET THE RESULTS */
  /*   if(Get_MAX8906_PM_ADDR(REG_ADC_MBATT_MSB, result, 2) != TRUE) */
  /*   { */
  /*       if(Get_MAX8906_PM_ADDR(REG_ADC_MBATT_MSB, result, 2) != TRUE) */
  /*       { */
  /*           // X can't be read */
  /*           return; */
  /*       } */
  /*   } */
  /*   voltage = (word)(result[0]<<4) | (word)(result[1]>>4); */
  /*   printk("[MAX8906]: MBATT = 0x%04X\n", voltage); */

  /*   if(Get_MAX8906_PM_ADDR(REG_ADC_BBATT_MSB, result, 2) != TRUE) */
  /*   { */
  /*       if(Get_MAX8906_PM_ADDR(REG_ADC_BBATT_MSB, result, 2) != TRUE) */
  /*       { */
  /*           // X can't be read */
  /*           return; */
  /*       } */
  /*   } */
  /*   voltage = (word)(result[0]<<4) | (word)(result[1]>>4); */
  /*   printk("[MAX8906]: BBATT = 0x%04X\n", voltage); */

  /*   if(Get_MAX8906_PM_ADDR(REG_ADC_VBUS_MSB, result, 2) != TRUE) */
  /*   { */
  /*       if(Get_MAX8906_PM_ADDR(REG_ADC_VBUS_MSB, result, 2) != TRUE) */
  /*       { */
  /*           // X can't be read */
  /*           return; */
  /*       } */
  /*   } */
  /*   voltage = (word)(result[0]<<4) | (word)(result[1]>>4); */
  /*   printk("[MAX8906]: VCC_USB = 0x%04X\n", voltage); */

    // switch measurement off
  /* Set_MAX8906_TSC_CONV_REG(MBATT_Measurement, NON_EN_REF_CONT); */
  /* Set_MAX8906_TSC_CONV_REG(BBATT_Measurement, NON_EN_REF_CONT); */
  /* Set_MAX8906_TSC_CONV_REG(VBUS_Measurement, NON_EN_REF_CONT); */
}




/*
 * mfd device init
 */

static int max8906_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct max8906_dev *max8906;
	struct max8906_platform_data *pdata = i2c->dev.platform_data;
	int ret = 0;

	max8906 = kzalloc(sizeof(struct max8906_dev), GFP_KERNEL);
	if (max8906 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, max8906);
	max8906->dev = &i2c->dev;
	max8906->i2c = i2c;
	max8906->type = id->driver_data;

	if (!pdata)
		goto err;

	max8906->wakeup = pdata->wakeup;

	mutex_init(&max8906->iolock);

	max8906_gpm_i2c_client = i2c_new_dummy(i2c->adapter, I2C_ADDR_GPM);
	max8906->gpm = max8906_gpm_i2c_client;
	i2c_set_clientdata(max8906->gpm, max8906);
	max8906_apm_i2c_client = i2c_new_dummy(i2c->adapter, I2C_ADDR_APM);
	max8906->apm = max8906_apm_i2c_client;
	i2c_set_clientdata(max8906->apm, max8906);
	max8906_adc_i2c_client = i2c_new_dummy(i2c->adapter, I2C_ADDR_ADC);
	max8906->adc = max8906_adc_i2c_client;
	i2c_set_clientdata(max8906->adc, max8906);
	max8906_rtc_i2c_client = i2c_new_dummy(i2c->adapter, I2C_ADDR_RTC);
	max8906->rtc = max8906_rtc_i2c_client;
	i2c_set_clientdata(max8906->rtc, max8906);
	pm_runtime_set_active(max8906->dev);

	mfd_add_devices(max8906->dev, -1, max8906_devs,
			ARRAY_SIZE(max8906_devs),
			NULL, 0);

	/*
	 * TODO: enable others (flash, muic, rtc, battery, ...) and
	 * check the return value
	 */

	if (ret < 0)
		goto err_mfd;

	return ret;

err_mfd:
	mfd_remove_devices(max8906->dev);
	i2c_unregister_device(max8906->gpm);
	i2c_unregister_device(max8906->apm);
	i2c_unregister_device(max8906->adc);
	i2c_unregister_device(max8906->rtc);


err:
	kfree(max8906);
#ifdef PMIC_EXTRA_DEBUG	
	printk("%s -> return %i\n",	__FUNCTION__, ret);
#endif	
	return ret;
}

static int max8906_i2c_remove(struct i2c_client *i2c)
{
	struct max8906_dev *max8906 = i2c_get_clientdata(i2c);

	mfd_remove_devices(max8906->dev);
	i2c_unregister_device(max8906->gpm);
	i2c_unregister_device(max8906->apm);
	i2c_unregister_device(max8906->adc);
	i2c_unregister_device(max8906->rtc);

	kfree(max8906);

	return 0;
}

static const struct i2c_device_id max8906_i2c_id[] = {
	{ "max8906", TYPE_MAX8906 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max8906_i2c_id);

struct max8906_reg_dump {
	u8	addr;
	u8	val;
};

#define SAVE_ITEM(x)	{ .addr = (x), .val = 0x0, }
static struct max8906_reg_dump max8906_dump[] = {
//	SAVE_ITEM(MAX8906_REG_A),
//	SAVE_ITEM(MAX8906_REG_B)
};

/* Save registers before hibernation */
static int max8906_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max8906_dev *max8906 = i2c_get_clientdata(i2c);
	int i;
// TODO: implement register dump
/*
	for (i = 0; i < ARRAY_SIZE(max8906_dumpaddr_pmic); i++)
		max8906_read_reg(i2c, max8906_dumpaddr_pmic[i],
				&max8906->reg_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max8906_dumpaddr_muic); i++)
		max8906_read_reg(i2c, max8906_dumpaddr_muic[i],
				&max8906->reg_dump[i + MAX8906_REG_PMIC_END]);

	for (i = 0; i < ARRAY_SIZE(max8906_dumpaddr_haptic); i++)
		max8906_read_reg(i2c, max8906_dumpaddr_haptic[i],
				&max8906->reg_dump[i + MAX8906_REG_PMIC_END +
				max8906_MUIC_REG_END]);
*/
	return 0;
}

/* Restore registers after hibernation */
static int max8906_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max8906_dev *max8906 = i2c_get_clientdata(i2c);
	int i;
// TODO: implement register restore
/*
	for (i = 0; i < ARRAY_SIZE(max8906_dumpaddr_pmic); i++)
		max8906_write_reg(i2c, max8906_dumpaddr_pmic[i],
				max8906->reg_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max8906_dumpaddr_muic); i++)
		max8906_write_reg(i2c, max8906_dumpaddr_muic[i],
				max8906->reg_dump[i + MAX8906_REG_PMIC_END]);

	for (i = 0; i < ARRAY_SIZE(max8906_dumpaddr_haptic); i++)
		max8906_write_reg(i2c, max8906_dumpaddr_haptic[i],
				max8906->reg_dump[i + MAX8906_REG_PMIC_END +
				max8906_MUIC_REG_END]);
*/
	return 0;
}

const struct dev_pm_ops max8906_pm = {
// TODO: add suspend and resume like in max8998 driver
	.freeze = max8906_freeze,
	.restore = max8906_restore,
};

static struct i2c_driver max8906_i2c_driver = {
	.driver = {
		.name = "max8906",
		.owner = THIS_MODULE,
		.pm = &max8906_pm,
	},
	.probe = max8906_i2c_probe,
	.remove = max8906_i2c_remove,
	.id_table = max8906_i2c_id,
};

static int pmic_init_status = 0;
int is_pmic_initialized(void)
{
	return pmic_init_status;
}

static int __init max8906_i2c_init(void)
{
	int ret;
	ret = i2c_add_driver(&max8906_i2c_driver);
	printk("MAX8906: %s retval = %d\n",__FUNCTION__,ret);
	pmic_init_status = 1;

	max8906_debug_print();

	return ret;
}

/* init early so consumer devices can complete system boot */
subsys_initcall(max8906_i2c_init);

static void __exit max8906_i2c_exit(void)
{
	i2c_del_driver(&max8906_i2c_driver);
	pmic_init_status = 0;
}

//module_init(max8906_i2c_init);
module_exit(max8906_i2c_exit);


MODULE_DESCRIPTION("MAXIM 8906 multi-function core driver");
MODULE_AUTHOR("dopi <dopi711@googlemail.com>");
MODULE_LICENSE("GPL");
