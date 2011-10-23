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

extern max8906_register_type  max8906reg[ENDOFREG];
extern max8906_function_type  max8906pm[ENDOFPM];
extern max8906_regulator_name_type regulator_name[NUMOFREG];

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
	{ .name = "max8906-rtc", },
	{ .name = "max8906-battery", },
};


/*
 * I2C Interface
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



/* MAX8906 voltage table */
static const unsigned int arm_voltage_table[61] = {
	725, 750, 775, 800, 825, 850, 875, 900,		/* 0 ~ 7 */
	925, 950, 975, 1000, 1025, 1050, 1075, 1100,	/* 8 ~ 15 */
	1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300,	/* 16 ~ 23 */
	1325, 1350, 1375, 1400, 1425, 1450, 1475, 1500,	/* 24 ~ 31 */
	1525, 1550, 1575, 1600, 1625, 1650, 1675, 1700,	/* 32 ~ 39 */
	1725, 1750, 1775, 1800, 1825, 1850, 1875, 1900,	/* 40 ~ 47 */
	1925, 1950, 1975, 2000, 2025, 2050, 2075, 2100,	/* 48 ~ 55 */
	2125, 2150, 2175, 2200, 2225,				/* 56 ~ 60 */
};

static const unsigned int int_voltage_table[3] = {
	1050, 1200, 1300,
};

/*===========================================================================

      P O W E R     M A N A G E M E N T     S E C T I O N

===========================================================================*/

/*===========================================================================

FUNCTION Set_MAX8906_PM_REG                                

DESCRIPTION
    This function write the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    value   :   the value for reg_num.
                This is aligned to the right side of the return value

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8906_PM_REG(CHGENB, onoff);

===========================================================================*/
boolean Set_MAX8906_PM_REG(max8906_pm_function_type reg_num, byte value)
{
    byte reg_buff;

    if(pmic_read(max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Register Read command failed
#ifdef CONFIG_MAX8906_VERBOSE_LOGGING
	pr_info(PREFIX "read from slave_add 0x%x, reg 0x%x failed = 0x%x \n", __func__, max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, reg_buff);
#endif
        return FALSE;
    }
#ifdef CONFIG_MAX8906_VERBOSE_LOGGING
	pr_info(PREFIX "read from slave_add 0x%x, reg 0x%x succeeded = 0x%x \n", __func__, max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, reg_buff);
#endif

    reg_buff = (reg_buff & max8906pm[reg_num].clear) | (value << max8906pm[reg_num].shift);
    if(pmic_write(max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Register Write command failed
#ifdef CONFIG_MAX8906_VERBOSE_LOGGING
	pr_info(PREFIX "write (0x%x) to slave_add 0x%x, reg 0x%x failed \n", __func__, reg:buff, max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr);
#endif
        return FALSE;
    }
#ifdef CONFIG_MAX8906_VERBOSE_LOGGING
	pr_info(PREFIX "write (0x%x) to slave_add 0x%x, reg 0x%x succeeded \n", __func__, reg:buff, max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr);
#endif
    return TRUE;
}

/*===========================================================================

FUNCTION Get_MAX8906_PM_REG                                

DESCRIPTION
    This function read the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE
    reg_buff :  the value of selected register.
                reg_buff is aligned to the right side of the return value.

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_PM_REG(max8906_pm_function_type reg_num, byte *reg_buff)
{
    byte temp_buff;

    if(pmic_read(max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, &temp_buff, (byte)1) != PMIC_PASS)
    {
        // Register Read Command failed
        return FALSE;
    }

    *reg_buff = (temp_buff & max8906pm[reg_num].mask) >> max8906pm[reg_num].shift;

    return TRUE;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_ADDR                                

DESCRIPTION
    This function write the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    max8906_pm_register_type reg_addr    : the register address.
    byte *reg_buff   : the array for data of register to write.
 	byte length      : the number of the register 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_ADDR(max8906_pm_register_type reg_addr, byte *reg_buff, byte length)
{

    if(pmic_write(max8906reg[reg_addr].slave_addr, max8906reg[reg_addr].addr, reg_buff, length) != PMIC_PASS)
    {
        // Register Write command failed
        return FALSE;
    }
    return TRUE;

}


/*===========================================================================

FUNCTION Get_MAX8906_PM_ADDR                                

DESCRIPTION
    This function read the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    max8906_pm_register_type reg_addr   : the register address.
    byte *reg_buff  : the array for data of register to write.
 	byte length     : the number of the register 

RETURN VALUE
    byte *reg_buff : the pointer parameter for data of sequential registers
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_PM_ADDR(max8906_pm_register_type reg_addr, byte *reg_buff, byte length)
{
    if(reg_addr > ENDOFREG)
    {
        // Invalid Read Register
        return FALSE; // return error;
    }
    if(pmic_read(max8906reg[reg_addr].slave_addr, max8906reg[reg_addr].addr, reg_buff, length) != PMIC_PASS)
    {
        // Register Read command failed
        return FALSE;
    }
    return TRUE;
}


/*===========================================================================

FUNCTION Set_MAX8906_TSC_CONV_REG                                

DESCRIPTION
    This function write the value at the selected register for Touch-Screen
    Conversion Command.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    byte cmd   : a data(bit0~2) of register to write.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/

boolean Set_MAX8906_TSC_CONV_REG(max8906_pm_function_type reg_num, byte cmd)
{
    byte tsc_buff;

    if((reg_num < X_Drive) || (reg_num > AUX2_Measurement))
    {
        // Incorrect register for TSC
        return FALSE;
    }

    tsc_buff =  (max8906pm[reg_num].mask & (byte)(cmd));

    if(pmic_write(max8906pm[reg_num].slave_addr, max8906pm[reg_num].addr, &tsc_buff, 1) != PMIC_PASS)
    {
        // Register Write command failed
        return FALSE;
    }
    return TRUE;
}



/*===========================================================================

FUNCTION Set_MAX8906_PM_Regulator_Active_Discharge

DESCRIPTION
    Enable/Disable Active Discharge for regulators.

INPUT PARAMETERS
    byte onoff : 0 = Active Discharge Enabled
                 1 = Active Discharge Disabled

    dword  regulators      : multiple regulators using "OR"
                             WBBCORE    WBBRF   APPS    IO      MEM     WBBMEM  
                             WBBIO      WBBANA  RFRXL   RFTXL   RFRXH   RFTCXO  
                             LDOA       LDOB    LDOC    LDOD    SIMLT   SRAM    
                             CARD1      CARD2   MVT     BIAS    VBUS    USBTXRX 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8906_PM_Regulator_Active_Discharge( 1, WBBCORE | APPS | MEM | WBBMEM);
    If you want to select one or a few regulators, please use Set_MAX8906_PM_REG().
    That is, Set_MAX8906_PM_REG(nWCRADE, 0); // APPS uses SEQ7

===========================================================================*/
boolean Set_MAX8906_PM_Regulator_Active_Discharge(byte onoff, dword regulators)
{
    boolean status;
    int i;

    status = TRUE;

    for(i=0; i < NUMOFREG; i++)
    {
        if(regulator_name[i].reg_name | regulators)
        {
            if(Set_MAX8906_PM_REG(regulator_name[i].active_discharge, onoff) != TRUE)
            {
                status = FALSE;
            }
        }
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_Regulator_ENA_SRC

DESCRIPTION
    Control Enable source for regulators from Flexible Power Sequence between
    SEQ1 ~ SEQ7 and software enable.

INPUT PARAMETERS
    flex_power_seq_type sequencer : selected Sequence number for each regulator
                                         SEQ1 ~ SEQ7 or SW_CNTL

    dword  regulators      : multiple regulators using "OR"
                             WBBCORE    WBBRF   APPS    IO      MEM     WBBMEM  
                             WBBIO      WBBANA  RFRXL   RFTXL   RFRXH   RFTCXO  
                             LDOA       LDOB    LDOC    LDOD    SIMLT   SRAM    
                             CARD1      CARD2   MVT     BIAS    VBUS    USBTXRX 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8906_PM_Regulator_ENA_SRC( SEQ1, WBBCORE | APPS | MEM | WBBMEM);
    If you want to select one or a few regulators, please use Set_MAX8906_PM_REG().
    That is, Set_MAX8906_PM_REG(APPSENSRC, 0x06); // APPS uses SEQ7

===========================================================================*/
boolean Set_MAX8906_PM_Regulator_ENA_SRC(flex_power_seq_type sequencer, dword regulators)
{
    boolean status;
    int i;

    status = TRUE;

    for(i=0; i < NUMOFREG; i++)
    {
        if(regulator_name[i].reg_name | regulators)
        {
            if(Set_MAX8906_PM_REG(regulator_name[i].ena_src_item, sequencer) != TRUE)
            {
                status = FALSE;
            }
        }
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_Regulator_SW_Enable

DESCRIPTION
    Enable/Disable Active Discharge for regulators.

INPUT PARAMETERS
    byte onoff : 
                 0 = Disabled
                 1 = Enabled

    dword  regulators      : multiple regulators using "OR"
                             WBBCORE    WBBRF   APPS    IO      MEM     WBBMEM  
                             WBBIO      WBBANA  RFRXL   RFTXL   RFRXH   RFTCXO  
                             LDOA       LDOB    LDOC    LDOD    SIMLT   SRAM    
                             CARD1      CARD2   MVT     BIAS    VBUS    USBTXRX 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8906_PM_Regulator_Active_Discharge( 1, WBBCORE | APPS | MEM | WBBMEM);
    If you want to select one or a few regulators, please use Set_MAX8906_PM_REG().
    That is, Set_MAX8906_PM_REG(nWCRADE, 0); // APPS uses SEQ7

===========================================================================*/
boolean Set_MAX8906_PM_Regulator_SW_Enable(byte onoff, dword regulators)
{
    boolean status;
    int i;

    status = TRUE;

    for(i=0; i < NUMOFREG; i++)
    {
        if(regulator_name[i].reg_name | regulators)
        {
            if(Set_MAX8906_PM_REG(regulator_name[i].sw_ena_dis, onoff) != TRUE)
            {
                status = FALSE;
            }
        }
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_PWR_SEQ_Timer_Period

DESCRIPTION
    Control the Timer Period between each sequencer event for Flexible Power
    Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for Timer Period
                                         SEQ1T ~ SEQ7T

    timer_period_type  value           : T_Period_20uS
                                         T_Period_40uS
                                         T_Period_80uS
                                         T_Period_160uS
                                         T_Period_320uS
                                         T_Period_640uS
                                         T_Period_1280uS
                                         T_Period_2560uS

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_PWR_SEQ_Timer_Period(max8906_pm_function_type cntl_item, timer_period_type value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1T:    case SEQ2T:
    case SEQ3T:    case SEQ4T:
    case SEQ5T:    case SEQ6T:
    case SEQ7T:
        if(Set_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Get_MAX8906_PM_PWR_SEQ_Timer_Period

DESCRIPTION
    Read the Timer Period between each sequencer event for Flexible Power
    Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for Timer Period
                                         SEQ1T, SEQ2T, SEQ3T, SEQ4T,
                                         SEQ5T, SEQ6T, SEQ7T

RETURN VALUE
    timer_period_type  value           : T_Period_20uS
                                         T_Period_40uS
                                         T_Period_80uS
                                         T_Period_160uS
                                         T_Period_320uS
                                         T_Period_640uS
                                         T_Period_1280uS
                                         T_Period_2560uS

    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_PM_PWR_SEQ_Timer_Period(max8906_pm_function_type cntl_item, timer_period_type *value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1T:    case SEQ2T:
	case SEQ3T:    case SEQ4T:
    case SEQ5T:    case SEQ6T:
	case SEQ7T:
		if(Get_MAX8906_PM_REG(cntl_item, (byte *)value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_PWR_SEQ_Ena_Src

DESCRIPTION
    Control the enable source for Flexible Power Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for enable source
                                         SEQ1SRC, SEQ2SRC, SEQ3SRC, SEQ4SRC, 
                                         SEQ5SRC, SEQ6SRC, SEQ7SRC

    byte value                         : 
                            SEQ1SRC = 0 : SYSEN hardware input
                                      1 : PWREN hardware input
                                      2 : SEQ1EN software bit
                                         
                            SEQ2SRC = 0 : PWREN hardware input
                                      1 : SYSEN hardware input
                                      2 : SEQ2EN software bit
                                         
                            SEQ3SRC = 0 : WBBEN hardware input
                                      1 : reserved
                                      2 : SEQ3EN software bit
                                         
                            SEQ4SRC = 0 : TCXOEN hardware input
                                      1 : reserved
                                      2 : SEQ4EN software bit
                                         
                            SEQ5SRC = 0 : RFRXEN hardware input
                                      1 : reserved
                                      2 : SEQ5EN software bit
                                         
                            SEQ6SRC = 0 : RFTXEN hardware input
                                      1 : reserved
                                      2 : SEQ6EN software bit
                                         
                            SEQ7SRC = 0 : ENA hardware input
                                      1 : reserved
                                      2 : SEQ7EN software bit
                                         

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_PWR_SEQ_Ena_Src(max8906_pm_function_type cntl_item, byte value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1SRC:    case SEQ2SRC:
    case SEQ3SRC:    case SEQ4SRC:
    case SEQ5SRC:    case SEQ6SRC:
    case SEQ7SRC:
        if(Set_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Get_MAX8906_PM_PWR_SEQ_Ena_Src

DESCRIPTION
    Read the enable source for Flexible Power Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for enable source
                                         SEQ1SRC, SEQ2SRC, SEQ3SRC, SEQ4SRC, 
                                         SEQ5SRC, SEQ6SRC, SEQ7SRC

RETURN VALUE
    byte value                         : 
                            SEQ1SRC = 0 : SYSEN hardware input
                                      1 : PWREN hardware input
                                      2 : SEQ1EN software bit
                                         
                            SEQ2SRC = 0 : PWREN hardware input
                                      1 : SYSEN hardware input
                                      2 : SEQ2EN software bit
                                         
                            SEQ3SRC = 0 : WBBEN hardware input
                                      1 : reserved
                                      2 : SEQ3EN software bit
                                         
                            SEQ4SRC = 0 : TCXOEN hardware input
                                      1 : reserved
                                      2 : SEQ4EN software bit
                                         
                            SEQ5SRC = 0 : RFRXEN hardware input
                                      1 : reserved
                                      2 : SEQ5EN software bit
                                         
                            SEQ6SRC = 0 : RFTXEN hardware input
                                      1 : reserved
                                      2 : SEQ6EN software bit
                                         
                            SEQ7SRC = 0 : ENA hardware input
                                      1 : reserved
                                      2 : SEQ7EN software bit

    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_PM_PWR_SEQ_Ena_Src(max8906_pm_function_type cntl_item, byte *value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1SRC:    case SEQ2SRC:
    case SEQ3SRC:    case SEQ4SRC:
    case SEQ5SRC:    case SEQ6SRC:
    case SEQ7SRC:
        if(Get_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_PWR_SEQ_SW_Enable

DESCRIPTION
    Control the enable source for Flexible Power Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for enable source
                                         SEQ1EN, SEQ2EN, SEQ3EN, SEQ4EN, 
                                         SEQ5EN, SEQ6EN, SEQ7EN

    byte value :       0 = Disable regulators
                       1 = Enable regulators

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_PWR_SEQ_SW_Enable(max8906_pm_function_type cntl_item, byte value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1EN:    case SEQ2EN:
    case SEQ3EN:    case SEQ4EN:
    case SEQ5EN:    case SEQ6EN:
    case SEQ7EN:
        if(Set_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}


/*===========================================================================

FUNCTION Get_MAX8906_PM_PWR_SEQ_SW_Enable

DESCRIPTION
    Read the enable source for Flexible Power Sequencer.

INPUT PARAMETERS
    max8906_pm_function_type cntl_item : selected Sequence number for enable source
                                         SEQ1EN, SEQ2EN, SEQ3EN, SEQ4EN, 
                                         SEQ5EN, SEQ6EN, SEQ7EN


RETURN VALUE
    byte value :       0 = Disable regulators
                       1 = Enable regulators

    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 


===========================================================================*/
boolean Get_MAX8906_PM_PWR_SEQ_SW_Enable(max8906_pm_function_type cntl_item, byte *value)
{
    boolean status;

    status = TRUE;
    switch(cntl_item)
    {
    case SEQ1EN:    case SEQ2EN:
    case SEQ3EN:    case SEQ4EN:
    case SEQ5EN:    case SEQ6EN:
    case SEQ7EN:

        if(Get_MAX8906_PM_REG(cntl_item, value) != TRUE)
        {
            status =FALSE;
        }
        break;
    default:
        status = FALSE;
    }

    return status;
}



/*===========================================================================*/
boolean change_vcc_arm(int voltage)
{
	byte reg_value = 0;

#ifdef CONFIG_MAX8906_VERBOSE_LOGGING
	pr_info(PREFIX "%s:I: voltage: %d\n", __func__, voltage);
#endif

	if(voltage < arm_voltage_table[0] || voltage > arm_voltage_table[60]) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	if (voltage % 25) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	if(voltage < arm_voltage_table[16]) { //725 ~ 1100 mV
		for(reg_value = 0; reg_value <= 15; reg_value++) {
			if(arm_voltage_table[reg_value] == voltage) break;
		}
	}
	else if(voltage < arm_voltage_table[32]) {	//1125 ~ 1500 mV
		for(reg_value = 16; reg_value <= 31; reg_value++) {
			if(arm_voltage_table[reg_value] == voltage) break;
		}
	}
	else if(voltage < arm_voltage_table[48]) {	//1525 ~ 1900 mV
		for(reg_value = 32; reg_value <= 47; reg_value++) {
			if(arm_voltage_table[reg_value] == voltage) break;
		}
	}
	else if(voltage <= arm_voltage_table[60]) {	//1925 ~ 2225 mV
		for(reg_value = 48; reg_value <= 60; reg_value++) {
			if(arm_voltage_table[reg_value] == voltage) break;
		}
	}
	else {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

 	Set_MAX8906_PM_REG(T1AOST, 0x0);

	Set_MAX8906_PM_REG(T1APPS, reg_value);

	/* Start Voltage Change */
	Set_MAX8906_PM_REG(AGO, 0x01);

#ifdef CONFIG_MAX8906_VERBOSE_LOGGING
	pr_info(PREFIX "%s:I: Succeeded!\n", __func__);
#endif

	return TRUE;
}

boolean change_vcc_internal(int voltage)
{	
	byte reg_value = 0;

#ifdef CONFIG_MAX8906_VERBOSE_LOGGING
	pr_info(PREFIX "%s:I: voltage: %d\n", __func__, voltage);
#endif

	if(voltage < int_voltage_table[0] || voltage > int_voltage_table[2]) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	for(reg_value = 0; reg_value <= 2; reg_value++) {
		if(int_voltage_table[reg_value] == voltage) break;
	}

	if (reg_value > 2) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}
	if (!Set_MAX8906_PM_REG(MEMDVM, reg_value)) {
		pr_err(PREFIX "%s:E: set pmic reg fail(%d)\n", __func__, reg_value);
		return FALSE;
	}

#ifdef CONFIG_MAX8906_VERBOSE_LOGGING
	pr_info(PREFIX "%s:I: Succeeded!\n", __func__);
#endif

	return TRUE;
}

boolean set_pmic(pmic_pm_type pm_type, int value)
{
	boolean rc = FALSE;
	switch (pm_type) {
	case VCC_ARM:
		rc = change_vcc_arm(value);
		break;
	case VCC_INT:
		rc = change_vcc_internal(value);
		break;
	default:
		pr_err(PREFIX "%s:E: invalid pm_type: %d\n", __func__, pm_type);
		rc = FALSE;
		break;
	}
	return rc;
}

boolean get_pmic(pmic_pm_type pm_type, int *value)
{
	boolean rc = FALSE;
	byte reg_buff;
	*value = 0;

	switch (pm_type) {
	case VCC_ARM:
		if(! Get_MAX8906_PM_REG(T1APPS, &reg_buff)) {
			pr_err(PREFIX "%s:VCC_ARM: get pmic reg fail\n",
					__func__);
			return FALSE;
		}
		if((reg_buff) < 61)
			*value = arm_voltage_table[reg_buff];
		break;
	case VCC_INT:
		if(!Get_MAX8906_PM_REG(MEMDVM, &reg_buff))
		{
			pr_err(PREFIX "%s:VCC_INT: get pmic reg fail\n",
					__func__);
			return FALSE;
		}
		if((reg_buff) < 3)
			*value = int_voltage_table[reg_buff];
		break;
	default:
		pr_err(PREFIX "%s:E: invalid pm_type: %d\n", __func__, pm_type);
		rc = FALSE;
		break;
	}
        return rc;
}










/*===========================================================================

      I N I T    R O U T I N E

===========================================================================*/
/*===========================================================================

FUNCTION MAX8906_PM_init

DESCRIPTION
    When power up, MAX8906_PM_init will initialize the MAX8906 for each part

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8906_PM_init(void)
{
    // Set the LDO voltage
    // When PWRSL is connected to GND, the default voltage of MAX8906 is like this.
    // LDO1 = 2.6V    LDO2  = 2.6V   LDO3 = 2.85V   LDO4 = 2.85V
    // LDO5 = 2.85V    LDO6  = 2.85V  LDO7 = 2.85V   LDO8 = 2.85V
    // LDO9 = 2.85V   LDO10 = 2.85V  LDO11 = 1.8V   LDO14 = 2.80V

    // Set the LDO on/off function
    // the default value for MAX8906
    // SD1   = On   SD2   = On   SD3  = Off  LDO1  = On   LDO2   = On
    // LDO3  = Off  LDO4  = Off  LDO5 = Off  LDO6  = Off  LDO7   = Off
    // LDO8  = Off  LDO9 = Off  LDO10 = Off  LDO11 = Off  LDO12  = Off
    // LDO11 = Off  LDO12 = On  LDO13 = Off  LDO14 = Off  REFOUT = On 

     // if you use USB transceiver, Connect internal 1.5k pullup resistor.
     //Set_MAX8906_PM_USB_CNTL(USB_PU_EN,1);
     // enable SMPL function
     //Set_MAX8906_RTC_REG(WTSR_SMPL_CNTL_EN_SMPL, (byte)1);
     // enable WTSR function for soft reset
     //Set_MAX8906_RTC_REG(WTSR_SMPL_CNTL_EN_WTSR, (byte)1);
}

/*===================================================================================================================*/
/* MAX8906 I2C Interface                                                                                             */
/*===================================================================================================================*/



static struct i2c_driver max8906_i2c_driver;

static struct i2c_client *max8906_rtc_i2c_client = NULL;
static struct i2c_client *max8906_adc_i2c_client = NULL;
static struct i2c_client *max8906_gpm_i2c_client = NULL;
static struct i2c_client *max8906_apm_i2c_client = NULL;

/*
static unsigned short max8906_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short max8906_ignore[] = { I2C_CLIENT_END };
static unsigned short max8906_probe[] = { 	3, (MAX8906_RTC_ID >> 1), 
						3, (MAX8906_ADC_ID >> 1),
						3, (MAX8906_GPM_ID >> 1), 
						3, (MAX8906_APM_ID >> 1), 
						I2C_CLIENT_END };
*/
/*
static struct i2c_client_address_data max8906_addr_data = {
	.normal_i2c = max8906_normal_i2c,
	.ignore		= max8906_ignore,
	.probe		= max8906_probe,
};
*/

static int max8906_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

static int max8906_write(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = data;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	if ((client->addr == MAX8906_GPM_ID) && (reg>0x83)&&(reg < 0x8f)) {
	  pr_info(PREFIX "%s:I: addr 0x%02X reg 0x%02X Succeeded!\n", __func__, client->addr, reg);
	}
	return 0;
}

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

	if (max8906_read(client, reg, data) < 0) { 
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

	if (max8906_write(client, reg, *data) < 0) { 
		printk(KERN_ERR "%s -> Failed! (slaveaddr 0x%02x, reg 0x%02x, data 0x%02x)\n",
					__FUNCTION__, slaveaddr, reg, *data);
		return PMIC_FAIL;
	}	

	return PMIC_PASS;
}

unsigned int pmic_tsc_write(u8 slaveaddr, u8 *cmd)
{
	return 0;
}

unsigned int pmic_tsc_read(u8 slaveaddr, u8 *cmd)
{
	return 0;
}

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

	max8906->gpm = i2c_new_dummy(i2c->adapter, I2C_ADDR_GPM);
	i2c_set_clientdata(max8906->gpm, max8906);
	max8906->apm = i2c_new_dummy(i2c->adapter, I2C_ADDR_APM);
	i2c_set_clientdata(max8906->apm, max8906);
	max8906->adc = i2c_new_dummy(i2c->adapter, I2C_ADDR_ADC);
	i2c_set_clientdata(max8906->adc, max8906);
	max8906->rtc = i2c_new_dummy(i2c->adapter, I2C_ADDR_RTC);
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
