/*
 * max8906-irq.c - Interrupt controller support for MAX8906
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
 * This driver is based on max8998-irq.c
 * MyungJoo Ham <myungjoo.ham@samsung.com>
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mfd/max8906.h>
#include <linux/mfd/max8906-private.h>






max8906_irq_mask_type max8906_irq_init_array[NUMOFIRQ] = {

    { REG_ON_OFF_IRQ_MASK, ON_OFF_IRQ_M},
    { REG_CHG_IRQ1_MASK,   CHG_IRQ1_M  },
    { REG_CHG_IRQ2_MASK,   CHG_IRQ2_M  },
    { REG_RTC_IRQ_MASK,    RTC_IRQ_M   },
    { REG_TSC_INT_MASK,    TSC_INT_M   }
};

max8906_irq_table_type max8906_irq_table[ENDOFTIRQ+1] = {
    /* ON_OFF_IRQ */
	{ 0, SW_R     , NULL }, // IRQ_SW_R
    { 0, SW_F     , NULL }, // IRQ_SW_F
    { 0, SW_1SEC  , NULL }, // IRQ_SW_1SEC
    { 0, JIG_R    , NULL }, // IRQ_JIG_R
    { 0, JIG_F    , NULL }, // IRQ_JIG_F
    { 0, SW_3SEC  , NULL }, // IRQ_SW_3SEC
    { 0, MPL_EVENT, NULL }, // IRQ_MPL_EVENT
    /* CHG_IRQ1 */
    { 1, VAC_R   , NULL }, // IRQ_VAC_R
    { 1, VAC_F   , NULL }, // IRQ_VAC_F
    { 1, VAC_OVP , NULL }, // IRQ_VAC_OVP
    { 1, VBUS_F  , NULL }, // IRQ_VBUS_F
    { 1, VBUS_R  , NULL }, // IRQ_VBUS_R
    { 1, VBUS_OVP, NULL }, // IRQ_VBUS_OVP
    /* CHG_IRQ2 */
    { 2, CHG_TMR_FAULT, NULL }, // IRQ_CHG_TMR_FAULT
    { 2, CHG_TOPOFF   , NULL }, // IRQ_CHG_TOPOFF
    { 2, CHG_DONE     , NULL }, // IRQ_CHG_DONE
    { 2, CHG_RST      , NULL }, // IRQ_CHG_RST
    { 2, MBATTLOWR    , NULL }, // IRQ_MBATTLOWR
    { 2, MBATTLOWF    , NULL }, // IRQ_MBATTLOWF
    /* RTC_IRQ */
    { 3, ALARM0_R, NULL }, // IRQ_ALARM0_R
    { 3, ALARM1_R, NULL }, // IRQ_ALARM1_R
    /* TSC_STA_INT */
    { 4, nREF_OK , NULL }, // IRQ_nREF_OK
    { 4, nCONV_NS, NULL }, // IRQ_nCONV_NS
    { 4, CONV_S  , NULL }, // IRQ_CONV_S
    { 4, nTS_NS  , NULL }, // IRQ_nTS_NS
    { 4, nTS_S   , NULL }  // IRQ_nTS_S
};




/*===========================================================================

      I R Q    R O U T I N E

===========================================================================*/
/*===========================================================================

FUNCTION MAX8906_IRQ_init                                

DESCRIPTION
    Initialize the IRQ Mask register for the IRQ handler.

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8906_IRQ_init()
{
    int i;

    for(i=0; i<NUMOFIRQ; i++)
    {
        if(pmic_write(max8906reg[max8906_irq_init_array[i].irq_reg].slave_addr, 
                   max8906reg[max8906_irq_init_array[i].irq_reg].addr, &max8906_irq_init_array[i].irq_mask, (byte)1) != PMIC_PASS)
        {
            // Write IRQ Mask failed
        }
    }
}


/*===========================================================================

FUNCTION Set_MAX8906_PM_IRQ                                

DESCRIPTION
    When some irq mask is changed, this function can be used.
    If you send max8906_isr as null(0) point, it means that the irq is masked.
    If max8906_isr points some functions, it means that the irq is unmasked.

INPUT PARAMETERS
    irq_name                   : IRQ Mask register number
    void (*max8906_isr)(void) : If irq is happened, then this routine is running.

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_PM_IRQ(max8906_irq_type irq_name, void (*max8906_isr)(void))
{
    byte reg_buff;
    byte value;

    if(pmic_read(max8906pm[max8906_irq_table[irq_name].irq_reg].slave_addr, 
                 max8906pm[max8906_irq_table[irq_name].irq_reg].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Read IRQ register failed
        return FALSE;
    }

    if(max8906_isr == 0)
    {   // if max8906_isr is a null pointer
        value = 1;
        reg_buff = (reg_buff | max8906pm[max8906_irq_table[irq_name].irq_reg].mask);
        max8906_irq_table[irq_name].irq_ptr = NULL;
    }
    else
    {
        value = 0;
        reg_buff = (reg_buff & max8906pm[max8906_irq_table[irq_name].irq_reg].clear);
        max8906_irq_table[irq_name].irq_ptr = max8906_isr;
    }

    if(pmic_write(max8906pm[max8906_irq_table[irq_name].irq_reg].slave_addr, 
                  max8906pm[max8906_irq_table[irq_name].irq_reg].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Write IRQ register failed
        return FALSE;
    }

    return TRUE;
}


/*===========================================================================

FUNCTION MAX8906_PM_IRQ_isr                                

DESCRIPTION
    When nIRQ pin is asserted, this isr routine check the irq bit and then
    proper function is called.
    Irq register can be set although irq is masked.
    So, the isr routine shoud check the irq mask bit, too.

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8906_PM_IRQ_isr(void)
{
    byte pm_irq_reg[4];
    byte pm_irq_mask[4];
    byte irq_name;
    if(pmic_read(max8906reg[REG_ON_OFF_IRQ].slave_addr, max8906reg[REG_ON_OFF_IRQ].addr, &pm_irq_reg[0], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }
    if(pmic_read(max8906reg[REG_CHG_IRQ1].slave_addr, max8906reg[REG_CHG_IRQ1].addr, &pm_irq_reg[1], (byte)2) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }
    if(pmic_read(max8906reg[REG_RTC_IRQ].slave_addr, max8906reg[REG_RTC_IRQ].addr, &pm_irq_reg[3], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }

    if(pmic_read(max8906reg[REG_ON_OFF_IRQ_MASK].slave_addr, max8906reg[REG_ON_OFF_IRQ].addr, &pm_irq_reg[0], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }
    if(pmic_read(max8906reg[REG_CHG_IRQ1_MASK].slave_addr, max8906reg[REG_CHG_IRQ1].addr, &pm_irq_reg[1], (byte)2) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }
    if(pmic_read(max8906reg[REG_RTC_IRQ_MASK].slave_addr, max8906reg[REG_RTC_IRQ].addr, &pm_irq_reg[3], (byte)1) != PMIC_PASS)
    {
        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
        return; // return error
    }

    for(irq_name = START_IRQ; irq_name <= ENDOFIRQ; irq_name++)
    {
        if( (pm_irq_reg[max8906_irq_table[irq_name].item_num] & max8906pm[max8906_irq_table[irq_name].irq_reg].mask)
            && ( (pm_irq_mask[max8906_irq_table[irq_name].item_num] & max8906pm[max8906_irq_table[irq_name].irq_reg].mask) == 0)
            && (max8906_irq_table[irq_name].irq_ptr != 0) )
        {
            (max8906_irq_table[irq_name].irq_ptr)();
        }
    }
}


/*===========================================================================

FUNCTION MAX8906_PM_TIRQ_isr                                

DESCRIPTION
    When nTIRQ pin is asserted for Touch-Screen, this isr routine check the irq bit and then
    proper function is called.
    Irq register can be set although irq is masked.
    So, the isr routine shoud check the irq mask bit, too.

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8906_PM_TIRQ_isr(void)
{
    byte pm_irq_reg[4];
    byte pm_irq_mask[4];
    byte irq_name;

    if(pmic_read(max8906reg[REG_TSC_STA_INT].slave_addr, max8906reg[REG_TSC_STA_INT].addr, &pm_irq_reg[0], (byte)1) != PMIC_PASS)
    {

        MSG_HIGH("IRQ register isn't read", 0, 0, 0);
	 	return; // return error
    }

    if(pmic_read(max8906reg[REG_TSC_INT_MASK].slave_addr, max8906reg[REG_TSC_INT_MASK].addr, &pm_irq_reg[0], (byte)1) != PMIC_PASS)
    {
	 	MSG_HIGH("IRQ register isn't read", 0, 0, 0);
	 	return; // return error
    }


    for(irq_name = START_TIRQ; irq_name <= ENDOFTIRQ; irq_name++)
    {
        if( (pm_irq_reg[max8906_irq_table[irq_name].item_num] & max8906pm[max8906_irq_table[irq_name].irq_reg].mask)
            && ( (pm_irq_mask[max8906_irq_table[irq_name].item_num] & max8906pm[max8906_irq_table[irq_name].irq_reg].mask) == 0)
            && (max8906_irq_table[irq_name].irq_ptr != 0) )
        {
            (max8906_irq_table[irq_name].irq_ptr)();
        }
    }
}

