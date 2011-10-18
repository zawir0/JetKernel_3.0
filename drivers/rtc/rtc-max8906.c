/*
 * RTC driver for Maxim MAX8906
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
 * This driver is based on rtc-max8998.c by
 *  Minkyu Kang <mk7.kang@samsung.com>
 *  Joonyoung Shim <jy0922.shim@samsung.com>
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/bcd.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/mfd/max8906.h>
#include <linux/mfd/max8906-private.h>
#include <linux/delay.h>







/*===========================================================================

      R T C     S E C T I O N

===========================================================================*/
/*===========================================================================

FUNCTION Set_MAX8906_RTC                                

DESCRIPTION
    This function write the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8906_rtc_cmd_type :     TIMEKEEPER = timekeeper register 0x0~0x7
                               ALARM0     = alarm0 register 0x8~0xF
                               ALARM1     = alarm1 register 0x10~0x18

    byte* max8906_rtc_ptr : the write value for registers.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8906_RTC(max8906_rtc_cmd_type rtc_cmd,byte *max8906_rtc_ptr)
{
    byte reg;

    reg = (byte)rtc_cmd * 8;
#if 0 
	if(pmic_rtc_write(reg, max8906_rtc_ptr, (byte)8) != PMIC_PASS)
    {
        //Write RTC failed
        return FALSE;
    }
#endif    
	pr_info(PREFIX "%s:I: Failed! - dummy function!!!\n", __func__);
 	return TRUE;
}

/*===========================================================================

FUNCTION Get_MAX8906_RTC                                

DESCRIPTION
    This function read the value at the selected register address
    in the RTC section.

INPUT PARAMETERS
    max8906_rtc_cmd_type :     TIMEKEEPER = timekeeper register 0x0~0x7
                               ALARM0     = alarm0 register 0x8~0xF
                               ALARM1     = alarm1 register 0x10~0x18

    byte* max8906_rtc_ptr : the return value for registers.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8906_RTC(max8906_rtc_cmd_type rtc_cmd, byte *max8906_rtc_ptr)
{
    byte reg;

    reg = (byte)rtc_cmd * 8;
#if 1  
    if(pmic_read(0xd0, reg, max8906_rtc_ptr, (byte)8) != PMIC_PASS)
    {
        // Read RTC failed
	pr_info(PREFIX "%s:I: Failed!\n", __func__);
         return FALSE;
    }
#endif    
	pr_info(PREFIX "%s:I: Succeeded!\n", __func__);
	return TRUE;
}










static int __devexit max8906_rtc_remove(struct platform_device *pdev)
{
	struct max8906_rtc_info *info = platform_get_drvdata(pdev);

	if (info) {
		free_irq(info->irq, info);
		rtc_device_unregister(info->rtc_dev);
		kfree(info);
	}

	return 0;
}

static const struct platform_device_id max8906_rtc_id[] = {
	{ "max8906-rtc", TYPE_MAX8906 },
	{ }
};

static struct platform_driver max8906_rtc_driver = {
	.driver		= {
		.name	= "max8906-rtc",
		.owner	= THIS_MODULE,
	},
	.probe		= max8906_rtc_probe,
	.remove		= __devexit_p(max8906_rtc_remove),
	.id_table	= max8906_rtc_id,
};

static int __init max8906_rtc_init(void)
{
	return platform_driver_register(&max8906_rtc_driver);
}
module_init(max8906_rtc_init);

static void __exit max8906_rtc_exit(void)
{
	platform_driver_unregister(&max8906_rtc_driver);
}
module_exit(max8906_rtc_exit);

MODULE_AUTHOR("Dopi <dopi711@googlemail.com>");
MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("Maxim MAX8906 RTC driver");
MODULE_LICENSE("GPL");
