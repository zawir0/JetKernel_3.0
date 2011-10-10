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
 */

#ifndef _MAX8906_H_
#define _MAX8906_H_

enum {
	MAX8906_LDO2,
	MAX8906_LDO3,
	MAX8906_LDO4,
	MAX8906_LDO5,
	MAX8906_LDO6,
	MAX8906_LDO7,
	MAX8906_LDO8,
	MAX8906_LDO9,
	MAX8906_BUCK1,
	MAX8906_BUCK2,
	MAX8906_BUCK3
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
 */
struct max8906_platform_data {
	int				num_regulators;
	struct max8906_regulator_data	*regulators;
	unsigned int lbhyst;
	unsigned int lbth;
	unsigned int lben;
};

#endif /* _MAX8906_H_ */
