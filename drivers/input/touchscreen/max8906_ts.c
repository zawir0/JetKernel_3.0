/*
 * Driver for MAX8906 resistive touch screen controller 
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
 * Based on max11801_ts-c, mcs5000_ts.c by
 *  Zhang Jiejing <jiejing.zhang@freescale.com>
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/mfd/max8906.h>
#include <linux/mfd/max8906-private.h>







/*===========================================================================
 T O U C H   S C R E E N
===========================================================================*/
void Set_MAX8906_PM_TSC_init()
{
    // set interrupt & config

    // set averaging for average check

    // 
}

void Set_MAX8906_PM_TSC_detect_isr()
{
    // send TSC detect event
}


/*===========================================================================

FUNCTION Set_MAX8906_TSC_measurement

DESCRIPTION
    Read x, y, z1, and z2 coordinates for Touch_Screen.
    (z1 and z2 is used for 5-wire mode.)

INPUT PARAMETERS

RETURN VALUE
    tsc_coordinates : return value for inserting x, y, z1, and z2 coordinates

    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 


===========================================================================*/
boolean Set_MAX8906_TSC_measurement(max8906_coordinates_type *tsc_coordinates)
{
    boolean status;
    byte tsc_buff;
    byte result[2];
    
    // set x drive & measurement
	Set_MAX8906_TSC_CONV_REG(X_Drive, CONT);
	Set_MAX8906_TSC_CONV_REG(X_Measurement, CONT);

    // X Measurement done?
    do {
        status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
    } while ( !(status && (tsc_buff & nCONV_NS_M)) );

    // set y drive & measurement
	Set_MAX8906_TSC_CONV_REG(Y_Drive, CONT);
	Set_MAX8906_TSC_CONV_REG(Y_Measurement, CONT);

    // Y Measurement done?
    do {
        status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
    } while ( !(status && (tsc_buff & nCONV_NS_M)) );

#ifdef TSC_5_WIRE_MODE
    // set z1 drive & measurement
    Set_MAX8906_TSC_CONV_REG(Z1_Drive, CONT);
    Set_MAX8906_TSC_CONV_REG(Z1_Measurement, CONT);

    // Y Measurement done?
    do {
        status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
    } while ( !(status && (tsc_buff & nCONV_NS_M)) );

    // set z2 drive & measurement
    Set_MAX8906_TSC_CONV_REG(Z2_Drive, CONT);
    Set_MAX8906_TSC_CONV_REG(Z2_Measurement, CONT);

    // Y Measurement done?
    do {
        status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
    } while ( !(status && (tsc_buff & nCONV_NS_M)) );

#endif

    // read irq reg for clearing interrupt

    // read x & y coordinates if nTIRQ is low
    status = Get_MAX8906_PM_REG(nCONV_NS, &tsc_buff);
	if(tsc_buff & nTS_NS_MASK)
	{
        // No touch event detected on sensor now
        return FALSE;
    }

    if(Get_MAX8906_PM_ADDR(REG_ADC_X_MSB, result, 2) != TRUE)
    {
        if(Get_MAX8906_PM_ADDR(REG_ADC_X_MSB, result, 2) != TRUE)
        {
            // X can't be read
            return FALSE;
        }
    }
    tsc_coordinates->x = (word)(result[0]<<4) | (word)(result[1]>>4);

    if(Get_MAX8906_PM_ADDR(REG_ADC_Y_MSB, result, 2) != TRUE)
    {
        if(Get_MAX8906_PM_ADDR(REG_ADC_Y_MSB, result, 2) != TRUE)
        {
            // Y can't be read
            return FALSE;
        }
    }
    tsc_coordinates->y = (word)(result[0]<<4) | (word)(result[1]>>4);

#ifdef TSC_5_WIRE_MODE
    if(Get_MAX8906_PM_ADDR(REG_ADC_Z1_MSB, result, 2) != TRUE)
    {
        if(Get_MAX8906_PM_ADDR(REG_ADC_Z1_MSB, result, 2) != TRUE)
        {
            // Z1 can't be read
            return FALSE;
        }
    }
    tsc_coordinates->z1 = (word)(result[0]<<4) | (word)(result[1]>>4);

    if(Get_MAX8906_PM_ADDR(REG_ADC_Z2_MSB, result, 2) != TRUE)
    {
        if(Get_MAX8906_PM_ADDR(REG_ADC_Z2_MSB, result, 2) != TRUE)
        {
            // Z2 can't be read
            return FALSE;
        }
    }
    tsc_coordinates->z2 = (word)(result[0]<<4) | (word)(result[1]>>4);
#endif

    // set x drive & measurement
    Set_MAX8906_TSC_CONV_REG(X_Drive, NON_EN_REF_CONT);
    Set_MAX8906_TSC_CONV_REG(X_Measurement, NON_EN_REF_CONT);

    // set y drive & measurement
    Set_MAX8906_TSC_CONV_REG(Y_Drive, NON_EN_REF_CONT);
    Set_MAX8906_TSC_CONV_REG(Y_Measurement, NON_EN_REF_CONT);

#ifdef TSC_5_WIRE_MODE
    // set z1 drive & measurement
    Set_MAX8906_TSC_CONV_REG(Z1_Drive, CONT);
    Set_MAX8906_TSC_CONV_REG(Z1_Measurement, CONT);

    // set z2 drive & measurement
    Set_MAX8906_TSC_CONV_REG(Z2_Drive, CONT);
    Set_MAX8906_TSC_CONV_REG(Z2_Measurement, CONT);
#endif

    return TRUE;
}






/*===========================================================================

FUNCTION Get_MAX8906_TSC_CONV_REG

DESCRIPTION
    This function read the value at the selected register for Touch-Screen
    Conversion Command.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    byte cmd   : a data(bit0~2) of register to write.

RETURN VALUE
    byte *reg_buff : the pointer parameter for data of sequential registers
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_TSC_CONV_REG(max8906_pm_function_type reg_num, byte *cmd)
{
    byte tsc_buff = 0;

    if((reg_num < X_Drive) || (reg_num > AUX2_Measurement))
    {
        // Incorrect register for TSC
        return FALSE;
    }
    if(pmic_tsc_read(max8906pm[reg_num].slave_addr, &tsc_buff) != PMIC_PASS)
    {
        // Register Read Command failed
        return FALSE;
    }

    *cmd = (tsc_buff & max8906pm[reg_num].mask);

    return TRUE;
}



