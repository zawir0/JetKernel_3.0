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


/* MAX8906 each register info */
typedef const struct {
	const byte  slave_addr;
	const byte  addr;
} max8906_register_type;

/* MAX8906 each function info */
typedef const struct {
	const byte  slave_addr;
	const byte  addr;
	const byte  mask;
	const byte  clear;
	const byte  shift;
} max8906_function_type;





/* MAX8906 each function info */
typedef const struct {
    const dword  reg_name;
    const max8906_pm_function_type active_discharge;
    const max8906_pm_function_type  ena_src_item;
    const max8906_pm_function_type  sw_ena_dis;
} max8906_regulator_name_type;





#endif /* _MAX8906_H_ */
