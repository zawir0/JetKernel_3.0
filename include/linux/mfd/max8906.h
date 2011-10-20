/*
 * max8906.h - Public definitions for Maxim MAX8906 voltage regulator driver
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
 * this file is based on max8698.h, max8997.h max8998.h and max8906.h by 
 * Tomasz Figa <tomasz.figa at gmail.com>, 
 * MyungJoo Ham <myungjoo.ham@smasung.com>,
 * Kyungmin Park <kyungmin.park@samsung.com>
 * Marek Szyprowski <m.szyprowski@samsung.com>
 * Mark Underwood (04/04/2006 Maxim Integrated Product)
 */

#ifndef _MAX8906_H_
#define _MAX8906_H_

/*
 * Driver data
 */

struct max8906_data {
	struct device		*dev;
	struct i2c_client	*i2c_client;
	int			num_regulators;
	struct regulator_dev	**rdev;
	struct mutex		lock;
	int			ramp_rate;
};

enum {
	// Linear Regulators (LDOs)
	MAX8906_WBBCORE,
	MAX8906_WBBRF,
	MAX8906_APPS,
	MAX8906_IO,
	MAX8906_MEM,
	MAX8906_WBBMEM,
	MAX8906_WBBIO,
	MAX8906_WBBANA,
	MAX8906_RFRXL,
	MAX8906_RFTXL,
	MAX8906_RFRXH,
	MAX8906_RFTCXO,
	MAX8906_LDOA,
	MAX8906_LDOB,
	MAX8906_LDOC,
	MAX8906_LDOD,
	MAX8906_SIMLT,
	MAX8906_SRAM,
	MAX8906_CARD1,
	MAX8906_CARD2,
	MAX8906_MVT,
	MAX8906_BIAS,
	MAX8906_VBUS,
	MAX8906_USBTXRX,
	// Flexible Power Sequencers (DCDC BUCK)
	MAX8906_SEQ1,
	MAX8906_SEQ2,
	MAX8906_SEQ3,
	MAX8906_SEQ4,
	MAX8906_SEQ5,
	MAX8906_SEQ6,
	MAX8906_SEQ7,
	MAX8906_SW_CNTL
};

/**
 * max8906_regulator_data - regulator data
 * @id: regulator id
 * @initdata: regulator init data (contraints, supplies, ...)
 */
struct max8906_regulator_data {
	int				id;
	struct regulator_init_data	*initdata;
};

/**
 * struct max8906_board - packages regulator init data
 * @num_regulators: number of regultors used
 * @regulators: array of defined regulators
 * @lbhyst: Low Main-Battery Comparator Hysteresis register value
 * @lbth: Low Main-Battery threshold voltage register value
 * @lben: Enable Low Main-Battery alarm signal
 * @wakeup: Allow to wake up from suspend
 */
struct max8906_platform_data {
	int				num_regulators;
	struct max8906_regulator_data	*regulators;
	unsigned int lbhyst;
	unsigned int lbth;
	unsigned int lben;

	bool	wakeup;
};

#endif /* _MAX8906_H_ */
