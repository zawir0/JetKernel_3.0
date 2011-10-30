/*
 * max8906.c - Maxim MAX8906 voltage regulator driver
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
 * this driver is based on max8698.c, max8997.c and max8906.c by 
 * Tomasz Figa <tomasz.figa at gmail.com>, 
 * MyungJoo Ham <myungjoo.ham@smasung.com>,
 * Kyungmin Park <kyungmin.park@samsung.com>
 * Marek Szyprowski <m.szyprowski@samsung.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/mfd/max8906.h>
#include <linux/mfd/max8906-private.h>

extern int max8906_i2c_device_update(struct max8906_data*, u8, u8, u8);
extern int max8906_i2c_device_read(struct max8906_data*, u8 , u8*);

extern unsigned int pmic_read(u8, u8, u8*, u8);
extern unsigned int pmic_write(u8, u8, u8*, u8);

#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX "MAX8906: "
//#define PMIC_EXTRA_DEBUG




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




/*===========================================================================*/


/* MAX8906 arm voltage table */
static const unsigned int arm_voltage_table[61] = {
	725, 750, 775, 800, 825, 850, 875, 900,		/* 0 ~ 7 */
	925, 950, 975, 1000, 1025, 1050, 1075, 1100,	/* 8 ~ 15 */
	1125, 1150, 1175, 1200, 1225, 1250, 1275, 1300,	/* 16 ~ 23 */
	1325, 1350, 1375, 1400, 1425, 1450, 1475, 1500,	/* 24 ~ 31 */
	1525, 1550, 1575, 1600, 1625, 1650, 1675, 1700,	/* 32 ~ 39 */
	1725, 1750, 1775, 1800, 1825, 1850, 1875, 1900,	/* 40 ~ 47 */
	1925, 1950, 1975, 2000, 2025, 2050, 2075, 2100,	/* 48 ~ 55 */
	2125, 2150, 2175, 2200, 2225,			/* 56 ~ 60 */
};

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

/* MAX8906 internal voltage table */
static const unsigned int int_voltage_table[3] = {
	1050, 1200, 1300,
};

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

























/*
 * Voltage regulator
 */

struct max8906reg_data {
	struct device		*dev;
	struct max8906_dev	*iodev;
	int			num_regulators;
	struct regulator_dev	**rdev;

	u8                      buck1_vol[4]; /* voltages for selection */
	u8                      buck2_vol[2];
	unsigned int		buck1_idx; /* index to last changed voltage */
					   /* value in a set */
	unsigned int		buck2_idx;
};

struct voltage_map_desc {
	int min;
	int max;
	int step;
};

/* Voltage maps */
static const struct voltage_map_desc ldo5V_voltage_map_desc = {
	.min = 5000000,	.step = 100000,	.max = 5000000,
};
static const struct voltage_map_desc ldo3V3_voltage_map_desc = {
	.min = 3300000,	.step = 100000,	.max = 3300000,
};
static const struct voltage_map_desc ldo075_390_voltage_map_desc = {
	.min = 750000,	.step = 50000,	.max = 3900000,
};
static const struct voltage_map_desc ldo075_180_voltage_map_desc = {
	.min = 750000,	.step = 50000,	.max = 1800000,
};
static const struct voltage_map_desc ldo0725_180_voltage_map_desc = {
	.min = 725000,	.step = 25000,	.max = 1800000,
};
static const struct voltage_map_desc ldo180_330_voltage_map_desc = {
	.min = 1800000,	.step = 50000,	.max = 3300000,
};
static const struct voltage_map_desc ldo180_300_voltage_map_desc = {
	.min = 1800000,	.step = 50000,	.max = 3000000,
};
static const struct voltage_map_desc ldo120_330_voltage_map_desc = {
	.min = 1200000,	.step = 50000,	.max = 3300000,
};

static const struct voltage_map_desc buck0725_180_voltage_map_desc = {
	.min = 725000,	.step = 25000,	.max = 1800000,
};
static const struct voltage_map_desc buck075_390_voltage_map_desc = {
	.min = 750000,	.step = 50000,	.max = 3900000,
};
static const struct voltage_map_desc buck1V8_2V8_3V0_voltage_map_desc = {
	.min = 1800000,	.step = 200000,	.max = 3000000,
};
static const struct voltage_map_desc buck1V05_1V2_1V3_voltage_map_desc = {
	.min = 1050000,	.step = 150000,	.max = 1300000,
};

static const struct voltage_map_desc *ldo_voltage_map[] = {
	&ldo5V_voltage_map_desc,	/* LDO_VBUS */
	&ldo3V3_voltage_map_desc,	/* LDO_USBTXRX */
	&ldo075_390_voltage_map_desc,	/* LDO_RFRXL */
	&ldo075_390_voltage_map_desc,	/* LDO_RFRXH */
	&ldo075_390_voltage_map_desc,	/* LDO_FTXL */
	&ldo075_390_voltage_map_desc,	/* LDO_SIMLT */
	&ldo075_390_voltage_map_desc,	/* LDOA */
	&ldo075_390_voltage_map_desc,	/* LDOB */
	&ldo075_390_voltage_map_desc,	/* LDOC */
	&ldo075_390_voltage_map_desc,	/* LDOD */
	&ldo3V3_voltage_map_desc,	/* LDO_BIAS */
	&ldo075_390_voltage_map_desc,	/* LDO_RFTCXO */
	&ldo075_180_voltage_map_desc,	/* LDO_MVT */
	&ldo0725_180_voltage_map_desc,	/* LDO_SRAM */
	&ldo075_390_voltage_map_desc,	/* LDO_CARD1 */
	&ldo180_330_voltage_map_desc,	/* LDO_CARD2 */
	&ldo180_300_voltage_map_desc,	/* LDO_WBBANA */
	&ldo180_300_voltage_map_desc,	/* LDO_WBBIO */
	&ldo120_330_voltage_map_desc,	/* LDO_WBBMEM */
	&buck0725_180_voltage_map_desc,	/* BUCK_WBBCORE */
	&buck075_390_voltage_map_desc,	/* BUCK_WBBRF */
	&buck0725_180_voltage_map_desc, /* BUCK_APPS */
	&buck1V8_2V8_3V0_voltage_map_desc, /* BUCK_IO */
	&buck1V05_1V2_1V3_voltage_map_desc,/* BUCK_MEM */
};

static inline int max8906_get_ldo(struct regulator_dev *rdev)
{
	return rdev_get_id(rdev);
}

static int max8906_list_voltage(struct regulator_dev *rdev,
				unsigned int selector)
{
	const struct voltage_map_desc *desc;
	int ldo = max8906_get_ldo(rdev);
	int val;

	if (ldo >= ARRAY_SIZE(ldo_voltage_map))
		return -EINVAL;

	switch (ldo) {
	case MAX8906_BUCK_IO:
		switch (selector) {
		case 0 ... 2:
			val = 1800000;
			break;
		default:
			return -EINVAL;
		}
	case MAX8906_BUCK_MEM:
		switch (selector) {
		case 0:
			val = 1050000;
			break;
		case 1:
			val = 1200000;
			break;
		case 2:
			val = 1200000;
			break;
		default:
			return -EINVAL;
		}
	default:
		desc = ldo_voltage_map[ldo];
		if (desc == NULL)
			return -EINVAL;

		val = desc->min + desc->step * selector;
		if (val > desc->max)
			return -EINVAL;
	}

	return val;
}
/*
enum {
	MAX8906_REG_ONOFF1,
	MAX8906_REG_ONOFF2,
	MAX8906_REG_ADISCHG_EN1,
	MAX8906_REG_ADISCHG_EN2,
	MAX8906_REG_DVSARM12,
	MAX8906_REG_DVSARM34,
	MAX8906_REG_DVSINT12,
	MAX8906_REG_BUCK3,
	MAX8906_REG_LDO23,
	MAX8906_REG_LDO4,
	MAX8906_REG_LDO5,
	MAX8906_REG_LDO6,
	MAX8906_REG_LDO7,
	MAX8906_REG_LDO8_BKCHR,
	MAX8906_REG_LDO9,
	MAX8906_REG_LBCNFG
};
*/

static int max8906_get_enable_register(struct regulator_dev *rdev,
					int *reg, int *shift)
{
	int ldo = max8906_get_ldo(rdev);
/* FIXME
	switch (ldo) {
	case MAX8906_LDO_VBUS ... MAX8906_LDO5:
		*reg = MAX8906_REG_ONOFF1;
		*shift = 4 - (ldo - MAX8906_LDO2);
		break;
	case MAX8906_LDO6 ... MAX8906_LDO9:
		*reg = MAX8906_REG_ONOFF2;
		*shift = 7 - (ldo - MAX8906_LDO6);
		break;
	case MAX8906_BUCK1 ... MAX8906_BUCK3:
		*reg = MAX8906_REG_ONOFF1;
		*shift = 7 - (ldo - MAX8906_BUCK1);
		break;
	default:
		return -EINVAL;
	}
*/
	return 0;
}

static int max8906_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct max8906reg_data *max8906reg = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8906reg->iodev->i2c;
	int ret, reg, shift = 8;
	u8 val;

	ret = max8906_get_enable_register(rdev, &reg, &shift);
	if (ret)
		return ret;

	ret = max8906_i2c_device_read(max8906reg, reg, &val);
	if (ret)
		return ret;

	return val & (1 << shift);
}

static int max8906_ldo_enable(struct regulator_dev *rdev)
{
	struct max8906reg_data *max8906reg = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8906reg->iodev->i2c;
	int reg, shift = 8, ret;

	ret = max8906_get_enable_register(rdev, &reg, &shift);
	if (ret)
		return ret;

	return max8906_i2c_device_update(max8906reg, reg, 1<<shift, 1<<shift);
}

static int max8906_ldo_disable(struct regulator_dev *rdev)
{
	struct max8906reg_data *max8906reg = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8906reg->iodev->i2c;
	int reg, shift = 8, ret;

	ret = max8906_get_enable_register(rdev, &reg, &shift);
	if (ret)
		return ret;

	return max8906_i2c_device_update(max8906reg, reg, 0, 1<<shift);
}

static int max8906_get_voltage_register(struct regulator_dev *rdev,
				int *_reg, int *_shift, int *_mask)
{
	int ldo = max8906_get_ldo(rdev);
	int reg, shift = 0, mask = 0xff;

	switch (ldo) {
/*
	case MAX8906_LDO2 ... MAX8906_LDO3:
		reg = MAX8906_REG_LDO23;
		mask = 0xf;
		if (ldo == MAX8906_LDO3)
			shift = 4;
		break;
	case MAX8906_LDO4 ... MAX8906_LDO7:
		reg = MAX8906_REG_LDO4 + (ldo - MAX8906_LDO4);
		break;
	case MAX8906_LDO8:
		reg = MAX8906_REG_LDO8_BKCHR;
		mask = 0xf;
		shift = 4;
		break;
	case MAX8906_LDO9:
		reg = MAX8906_REG_LDO9;
		break;
	case MAX8906_BUCK1:
		reg = MAX8906_REG_DVSARM12;
		mask = 0xf;
		break;
	case MAX8906_BUCK2:
		reg = MAX8906_REG_DVSINT12;
		mask = 0xf;
		break;
	case MAX8906_BUCK3:
		reg = MAX8906_REG_BUCK3;
		break;
*/
	default:
		return -EINVAL;
	}

	*_reg = reg;
	*_shift = shift;
	*_mask = mask;

	return 0;
}

static int max8906_get_voltage(struct regulator_dev *rdev)
{
	struct max8906reg_data *max8906reg = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8906reg->iodev->i2c;
	int reg, shift = 0, mask, ret;
	u8 val;

	ret = max8906_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	ret = max8906_i2c_device_read(max8906reg, reg, &val);
	if (ret)
		return ret;

	val >>= shift;
	val &= mask;

	return max8906_list_voltage(rdev, val);
}

static int max8906_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV, unsigned *selector)
{
	struct max8906reg_data *max8906reg = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8906reg->iodev->i2c;
	const struct voltage_map_desc *desc;
	int ldo = max8906_get_ldo(rdev);
	int reg = 0, shift = 0, mask = 0, ret;
	int i = 0;
	int sel_uV;

	if (ldo >= ARRAY_SIZE(ldo_voltage_map))
		return -EINVAL;

	desc = ldo_voltage_map[ldo];
	if (desc == NULL)
		return -EINVAL;

	if (max_uV < desc->min || min_uV > desc->max)
		return -EINVAL;

	sel_uV = desc->min;
	while (sel_uV < min_uV && sel_uV < desc->max) {
		sel_uV += desc->step;
		++i;
	}

	if (sel_uV > max_uV)
		return -EINVAL;

	*selector = i;

	ret = max8906_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	return max8906_i2c_device_update(max8906reg, reg,
						i << shift, mask << shift);
}

static struct regulator_ops max8906_regulator_ops = {
	.list_voltage		= max8906_list_voltage,
	.is_enabled		= max8906_ldo_is_enabled,
	.enable			= max8906_ldo_enable,
	.disable		= max8906_ldo_disable,
	.get_voltage		= max8906_get_voltage,
	.set_voltage		= max8906_set_voltage,
	.set_suspend_enable	= max8906_ldo_enable,
	.set_suspend_disable	= max8906_ldo_disable,
};

static int max8906_set_buck_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV, unsigned *selector)
{
	struct max8906reg_data *max8906reg = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = max8906reg->iodev->i2c;
	const struct voltage_map_desc *desc;
	int prev_uV, sel_uV;
	int ldo = max8906_get_ldo(rdev), i = 0, ret;

	if (ldo >= ARRAY_SIZE(ldo_voltage_map))
		return -EINVAL;

	desc = ldo_voltage_map[ldo];
	if (desc == NULL)
		return -EINVAL;

	if (max_uV < desc->min || min_uV > desc->max)
		return -EINVAL;

	sel_uV = desc->min;
	while (sel_uV < min_uV && sel_uV < desc->max) {
		sel_uV += desc->step;
		++i;
	}

	if (sel_uV > max_uV)
		return -EINVAL;

	*selector = i;

	prev_uV = max8906_get_voltage(rdev);
/*
	switch (ldo) {
	case MAX8906_BUCK1:
		ret = max8906_i2c_device_update(max8906reg,
				MAX8906_REG_DVSARM12, (i << 4) | i, 0xff);
		if (ret)
			goto err;
		ret = max8906_i2c_device_update(max8906reg,
				MAX8906_REG_DVSARM34, (i << 4) | i, 0xff);
		if (ret)
			goto err;
		break;
	case MAX8906_BUCK2:
		ret = max8906_i2c_device_update(max8906reg,
				MAX8906_REG_DVSINT12, (i << 4) | i, 0xff);
		if (ret)
			goto err;
		break;
	}

	if (prev_uV < sel_uV) {
		while (prev_uV < sel_uV) {
			udelay(1);
			prev_uV += max8906reg->ramp_rate;
		}
	} else {
		while (prev_uV > sel_uV) {
			udelay(1);
			prev_uV -= max8906reg->ramp_rate;
		}
	}
*/
err:
	return ret;
}

static struct regulator_ops max8906_regulator_buck_ops = {
	.list_voltage		= max8906_list_voltage,
	.is_enabled		= max8906_ldo_is_enabled,
	.enable			= max8906_ldo_enable,
	.disable		= max8906_ldo_disable,
	.get_voltage		= max8906_get_voltage,
	.set_voltage		= max8906_set_buck_voltage,
	.set_suspend_enable	= max8906_ldo_enable,
	.set_suspend_disable	= max8906_ldo_disable,
};

static struct regulator_desc regulators[] = {
	{
		.name		= "LDOA",
		.id		= MAX8906_LDOA,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK_MEM",
		.id		= MAX8906_BUCK_MEM,
		.ops		= &max8906_regulator_buck_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	},
};


/*
 * platform driver
 */

static __devinit int max8906_pmic_probe(struct platform_device *pdev)
{
	struct max8906_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max8906_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct regulator_dev **rdev;
	struct max8906reg_data *max8906reg;
	struct i2c_client *i2c;
	int i, ret, size;

	if (!pdata) {
		dev_err(pdev->dev.parent, "No platform init data supplied\n");
		return -ENODEV;
	}

	max8906reg = kzalloc(sizeof(struct max8906reg_data), GFP_KERNEL);
	if (!max8906reg)
		return -ENOMEM;

	size = sizeof(struct regulator_dev *) * pdata->num_regulators;
	max8906reg->rdev = kzalloc(size, GFP_KERNEL);
	if (!max8906reg->rdev) {
		kfree(max8906reg);
		return -ENOMEM;
	}

	rdev = max8906reg->rdev;
	max8906reg->dev = &pdev->dev;
	max8906reg->iodev = iodev;
	max8906reg->num_regulators = pdata->num_regulators;
	platform_set_drvdata(pdev, max8906reg);
	i2c = max8906reg->iodev->i2c;

//	max8906reg->buck1_idx = pdata->buck1_default_idx;
//	max8906reg->buck2_idx = pdata->buck2_default_idx;

	/* NOTE: */
	/* For unused GPIO NOT marked as -1 (thereof equal to 0)  WARN_ON */
	/* will be displayed */

	/* Check if MAX8906 voltage selection GPIOs are defined */
//	if (gpio_is_valid(pdata->buck1_set1) &&
//	    gpio_is_valid(pdata->buck1_set2)) {
		/* Check if SET1 is not equal to 0 */
//		if (!pdata->buck1_set1) {
//			printk(KERN_ERR "MAX8906 SET1 GPIO defined as 0 !\n");
//			WARN_ON(!pdata->buck1_set1);
//			ret = -EIO;
//			goto err_free_mem;
//		}
		/* Check if SET2 is not equal to 0 */
/*
		if (!pdata->buck1_set2) {
			printk(KERN_ERR "MAX8906 SET2 GPIO defined as 0 !\n");
			WARN_ON(!pdata->buck1_set2);
			ret = -EIO;
			goto err_free_mem;
		}

		gpio_request(pdata->buck1_set1, "MAX8906 BUCK1_SET1");
		gpio_direction_output(pdata->buck1_set1,
				      max8906reg->buck1_idx & 0x1);


		gpio_request(pdata->buck1_set2, "MAX8906 BUCK1_SET2");
		gpio_direction_output(pdata->buck1_set2,
				      (max8906reg->buck1_idx >> 1) & 0x1);
*/
		/* Set predefined value for BUCK1 register 1 */
/*
		i = 0;
		while (buck12_voltage_map_desc.min +
		       buck12_voltage_map_desc.step*i
		       < (pdata->buck1_voltage1 / 1000))
			i++;
		max8906reg->buck1_vol[0] = i;
		ret = max8906_write_reg(i2c, MAX8906_REG_BUCK1_VOLTAGE1, i);
		if (ret)
			goto err_free_mem;
*/
		/* Set predefined value for BUCK1 register 2 */
/*
		i = 0;
		while (buck12_voltage_map_desc.min +
		       buck12_voltage_map_desc.step*i
		       < (pdata->buck1_voltage2 / 1000))
			i++;

		max8906reg->buck1_vol[1] = i;
		ret = max8906_write_reg(i2c, MAX8906_REG_BUCK1_VOLTAGE2, i);
		if (ret)
			goto err_free_mem;
*/
		/* Set predefined value for BUCK1 register 3 */
/*
		i = 0;
		while (buck12_voltage_map_desc.min +
		       buck12_voltage_map_desc.step*i
		       < (pdata->buck1_voltage3 / 1000))
			i++;

		max8906reg->buck1_vol[2] = i;
		ret = max8906_write_reg(i2c, MAX8906_REG_BUCK1_VOLTAGE3, i);
		if (ret)
			goto err_free_mem;
*/
		/* Set predefined value for BUCK1 register 4 */
/*
		i = 0;
		while (buck12_voltage_map_desc.min +
		       buck12_voltage_map_desc.step*i
		       < (pdata->buck1_voltage4 / 1000))
			i++;

		max8906reg->buck1_vol[3] = i;
		ret = max8906_write_reg(i2c, MAX8906_REG_BUCK1_VOLTAGE4, i);
		if (ret)
			goto err_free_mem;

	}
*/
//	if (gpio_is_valid(pdata->buck2_set3)) {
		/* Check if SET3 is not equal to 0 */
/*
		if (!pdata->buck2_set3) {
			printk(KERN_ERR "MAX8906 SET3 GPIO defined as 0 !\n");
			WARN_ON(!pdata->buck2_set3);
			ret = -EIO;
			goto err_free_mem;
		}
		gpio_request(pdata->buck2_set3, "MAX8906 BUCK2_SET3");
		gpio_direction_output(pdata->buck2_set3,
				      max8906reg->buck2_idx & 0x1);
*/
		/* BUCK2 register 1 */
/*
		i = 0;
		while (buck12_voltage_map_desc.min +
		       buck12_voltage_map_desc.step*i
		       < (pdata->buck2_voltage1 / 1000))
			i++;
		max8906reg->buck2_vol[0] = i;
		ret = max8906_write_reg(i2c, MAX8906_REG_BUCK2_VOLTAGE1, i);
		if (ret)
			goto err_free_mem;
*/
		/* BUCK2 register 2 */
/*
		i = 0;
		while (buck12_voltage_map_desc.min +
		       buck12_voltage_map_desc.step*i
		       < (pdata->buck2_voltage2 / 1000))
			i++;
		printk(KERN_ERR "i2:%d, buck2_idx:%d\n", i, max8906reg->buck2_idx);
		max8906reg->buck2_vol[1] = i;
		ret = max8906_write_reg(i2c, MAX8906_REG_BUCK2_VOLTAGE2, i);
		if (ret)
			goto err_free_mem;
	}

	for (i = 0; i < pdata->num_regulators; i++) {
		const struct voltage_map_desc *desc;
		int id = pdata->regulators[i].id;
		int index = id - MAX8906_LDO2;

		desc = ldo_voltage_map[id];
		if (desc && regulators[index].ops != &max8906_others_ops) {
			int count = (desc->max - desc->min) / desc->step + 1;
			regulators[index].n_voltages = count;
		}
		rdev[i] = regulator_register(&regulators[index], max8906reg->dev,
				pdata->regulators[i].initdata, max8906reg);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(max8906reg->dev, "regulator init failed\n");
			rdev[i] = NULL;
			goto err;
		}
	}
*/

	return 0;
err:
	for (i = 0; i < max8906reg->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

err_free_mem:
	kfree(max8906reg->rdev);
	kfree(max8906reg);

	return ret;
}

static int __devexit max8906_pmic_remove(struct platform_device *pdev)
{
	struct max8906reg_data *max8906reg = platform_get_drvdata(pdev);
	struct regulator_dev **rdev = max8906reg->rdev;
	int i;

	for (i = 0; i < max8906reg->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

	kfree(max8906reg->rdev);
	kfree(max8906reg);

	return 0;
}

static const struct platform_device_id max8906_pmic_id[] = {
	{ "max8906-pmic", TYPE_MAX8906 },
	{ }
};
MODULE_DEVICE_TABLE(platform, max8906_pmic_id);

static struct platform_driver max8906_pmic_driver = {
	.driver = {
		.name = "max8906-pmic",
		.owner = THIS_MODULE,
	},
	.probe = max8906_pmic_probe,
	.remove = __devexit_p(max8906_pmic_remove),
	.id_table = max8906_pmic_id,
};

static int __init max8906_pmic_init(void)
{
	return platform_driver_register(&max8906_pmic_driver);
}
subsys_initcall(max8906_pmic_init);

static void __exit max8906_pmic_cleanup(void)
{
	platform_driver_unregister(&max8906_pmic_driver);
}
module_exit(max8906_pmic_cleanup);

MODULE_DESCRIPTION("Maxim 8906 voltage regulator driver");
MODULE_AUTHOR("Dopi <dopi711 at googlemail.com>");
MODULE_LICENSE("GPLv2");
