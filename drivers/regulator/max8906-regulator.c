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

extern max8906_register_type  max8906reg[ENDOFREG];
extern max8906_function_type  max8906pm[ENDOFPM];
extern max8906_regulator_name_type regulator_name[NUMOFREG];
extern int max8906_i2c_device_update(struct max8906_data*, u8, u8, u8);

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
static const struct voltage_map_desc ldo23_voltage_map_desc = {
	.min = 800000,	.step = 50000,	.max = 1300000,
};
static const struct voltage_map_desc ldo45679_voltage_map_desc = {
	.min = 1600000,	.step = 100000,	.max = 3600000,
};
static const struct voltage_map_desc ldo8_voltage_map_desc = {
	.min = 3000000,	.step = 100000,	.max = 3600000,
};
static const struct voltage_map_desc buck12_voltage_map_desc = {
	.min = 750000,	.step = 50000,	.max = 1500000,
};
static const struct voltage_map_desc buck3_voltage_map_desc = {
	.min = 1600000,	.step = 100000,	.max = 3600000,
};

static const struct voltage_map_desc *ldo_voltage_map[] = {
	&ldo23_voltage_map_desc,	/* LDO2 */
	&ldo23_voltage_map_desc,	/* LDO3 */
	&ldo45679_voltage_map_desc,	/* LDO4 */
	&ldo45679_voltage_map_desc,	/* LDO5 */
	&ldo45679_voltage_map_desc,	/* LDO6 */
	&ldo45679_voltage_map_desc,	/* LDO7 */
	&ldo8_voltage_map_desc,		/* LDO8 */
	&ldo45679_voltage_map_desc,	/* LDO9 */
	&buck12_voltage_map_desc,	/* BUCK1 */
	&buck12_voltage_map_desc,	/* BUCK2 */
	&buck3_voltage_map_desc,	/* BUCK3 */
};

/*
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

	desc = ldo_voltage_map[ldo];
	if (desc == NULL)
		return -EINVAL;

	val = desc->min + desc->step * selector;
	if (val > desc->max)
		return -EINVAL;

	return val;
}

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

static int max8906_get_enable_register(struct regulator_dev *rdev,
					int *reg, int *shift)
{
	int ldo = max8906_get_ldo(rdev);

	switch (ldo) {
	case MAX8906_LDO2 ... MAX8906_LDO5:
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

	return 0;
}

static int max8906_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct max8906reg_data *max8906 = rdev_get_drvdata(rdev);
	int ret, reg, shift = 8;
	u8 val;

	ret = max8906_get_enable_register(rdev, &reg, &shift);
	if (ret)
		return ret;

	ret = max8906_i2c_device_read(max8906, reg, &val);
	if (ret)
		return ret;

	return val & (1 << shift);
}

static int max8906_ldo_enable(struct regulator_dev *rdev)
{
	struct max8906reg_data *max8906 = rdev_get_drvdata(rdev);
	int reg, shift = 8, ret;

	ret = max8906_get_enable_register(rdev, &reg, &shift);
	if (ret)
		return ret;

	return max8906_i2c_device_update(max8906, reg, 1<<shift, 1<<shift);
}

static int max8906_ldo_disable(struct regulator_dev *rdev)
{
	struct max8906reg_data *max8906 = rdev_get_drvdata(rdev);
	int reg, shift = 8, ret;

	ret = max8906_get_enable_register(rdev, &reg, &shift);
	if (ret)
		return ret;

	return max8906_i2c_device_update(max8906, reg, 0, 1<<shift);
}

static int max8906_get_voltage_register(struct regulator_dev *rdev,
				int *_reg, int *_shift, int *_mask)
{
	int ldo = max8906_get_ldo(rdev);
	int reg, shift = 0, mask = 0xff;

	switch (ldo) {
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
	struct max8906reg_data *max8906 = rdev_get_drvdata(rdev);
	int reg, shift = 0, mask, ret;
	u8 val;

	ret = max8906_get_voltage_register(rdev, &reg, &shift, &mask);
	if (ret)
		return ret;

	ret = max8906_i2c_device_read(max8906, reg, &val);
	if (ret)
		return ret;

	val >>= shift;
	val &= mask;

	return max8906_list_voltage(rdev, val);
}

static int max8906_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV, unsigned *selector)
{
	struct max8906reg_data *max8906 = rdev_get_drvdata(rdev);
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

	return max8906_i2c_device_update(max8906, reg,
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

static int max8906_set_buck12_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV, unsigned *selector)
{
	struct max8906reg_data *max8906 = rdev_get_drvdata(rdev);
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

	switch (ldo) {
	case MAX8906_BUCK1:
		ret = max8906_i2c_device_update(max8906,
				MAX8906_REG_DVSARM12, (i << 4) | i, 0xff);
		if (ret)
			goto err;
		ret = max8906_i2c_device_update(max8906,
				MAX8906_REG_DVSARM34, (i << 4) | i, 0xff);
		if (ret)
			goto err;
		break;
	case MAX8906_BUCK2:
		ret = max8906_i2c_device_update(max8906,
				MAX8906_REG_DVSINT12, (i << 4) | i, 0xff);
		if (ret)
			goto err;
		break;
	}

	if (prev_uV < sel_uV) {
		while (prev_uV < sel_uV) {
			udelay(1);
			prev_uV += max8906->ramp_rate;
		}
	} else {
		while (prev_uV > sel_uV) {
			udelay(1);
			prev_uV -= max8906->ramp_rate;
		}
	}

err:
	return ret;
}

static struct regulator_ops max8906_regulator_buck12_ops = {
	.list_voltage		= max8906_list_voltage,
	.is_enabled		= max8906_ldo_is_enabled,
	.enable			= max8906_ldo_enable,
	.disable		= max8906_ldo_disable,
	.get_voltage		= max8906_get_voltage,
	.set_voltage		= max8906_set_buck12_voltage,
	.set_suspend_enable	= max8906_ldo_enable,
	.set_suspend_disable	= max8906_ldo_disable,
};

static struct regulator_desc regulators[] = {
	{
		.name		= "LDO2",
		.id		= MAX8906_LDO2,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO3",
		.id		= MAX8906_LDO3,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO4",
		.id		= MAX8906_LDO4,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO5",
		.id		= MAX8906_LDO5,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO6",
		.id		= MAX8906_LDO6,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO7",
		.id		= MAX8906_LDO7,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO8",
		.id		= MAX8906_LDO8,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "LDO9",
		.id		= MAX8906_LDO9,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK1",
		.id		= MAX8906_BUCK1,
		.ops		= &max8906_regulator_buck12_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK2",
		.id		= MAX8906_BUCK2,
		.ops		= &max8906_regulator_buck12_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "BUCK3",
		.id		= MAX8906_BUCK3,
		.ops		= &max8906_regulator_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	},
};
*/

/*
 * platform driver
 */

static __devinit int max8906_pmic_probe(struct platform_device *pdev)
{
	struct max8906_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max8906_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct regulator_dev **rdev;
	struct max8906reg_data *max8906;
	struct i2c_client *i2c;
	int i, ret, size;

	if (!pdata) {
		dev_err(pdev->dev.parent, "No platform init data supplied\n");
		return -ENODEV;
	}

	max8906 = kzalloc(sizeof(struct max8906reg_data), GFP_KERNEL);
	if (!max8906)
		return -ENOMEM;

	size = sizeof(struct regulator_dev *) * pdata->num_regulators;
	max8906->rdev = kzalloc(size, GFP_KERNEL);
	if (!max8906->rdev) {
		kfree(max8906);
		return -ENOMEM;
	}

	rdev = max8906->rdev;
	max8906->dev = &pdev->dev;
	max8906->iodev = iodev;
	max8906->num_regulators = pdata->num_regulators;
	platform_set_drvdata(pdev, max8906);
	i2c = max8906->iodev->i2c;

//	max8906->buck1_idx = pdata->buck1_default_idx;
//	max8906->buck2_idx = pdata->buck2_default_idx;

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
				      max8906->buck1_idx & 0x1);


		gpio_request(pdata->buck1_set2, "MAX8906 BUCK1_SET2");
		gpio_direction_output(pdata->buck1_set2,
				      (max8906->buck1_idx >> 1) & 0x1);
*/
		/* Set predefined value for BUCK1 register 1 */
/*
		i = 0;
		while (buck12_voltage_map_desc.min +
		       buck12_voltage_map_desc.step*i
		       < (pdata->buck1_voltage1 / 1000))
			i++;
		max8906->buck1_vol[0] = i;
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

		max8906->buck1_vol[1] = i;
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

		max8906->buck1_vol[2] = i;
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

		max8906->buck1_vol[3] = i;
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
				      max8906->buck2_idx & 0x1);
*/
		/* BUCK2 register 1 */
/*
		i = 0;
		while (buck12_voltage_map_desc.min +
		       buck12_voltage_map_desc.step*i
		       < (pdata->buck2_voltage1 / 1000))
			i++;
		max8906->buck2_vol[0] = i;
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
		printk(KERN_ERR "i2:%d, buck2_idx:%d\n", i, max8906->buck2_idx);
		max8906->buck2_vol[1] = i;
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
		rdev[i] = regulator_register(&regulators[index], max8906->dev,
				pdata->regulators[i].initdata, max8906);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(max8906->dev, "regulator init failed\n");
			rdev[i] = NULL;
			goto err;
		}
	}
*/

	return 0;
err:
	for (i = 0; i < max8906->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

err_free_mem:
	kfree(max8906->rdev);
	kfree(max8906);

	return ret;
}

static int __devexit max8906_pmic_remove(struct platform_device *pdev)
{
	struct max8906reg_data *max8906 = platform_get_drvdata(pdev);
	struct regulator_dev **rdev = max8906->rdev;
	int i;

	for (i = 0; i < max8906->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

	kfree(max8906->rdev);
	kfree(max8906);

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
