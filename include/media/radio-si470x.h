/*
 * include/media/radio-si470x.h
 *
 * Board related data definitions for Si470z fm radio chip.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */

#ifndef __MEDIA_RADIO_SI470x_H__
#define __MEDIA_RADIO_SI470x_H__

 struct si470x_platform_datas {
	int gpio_fm_on;
	int gpio_fm_reset;
};

#endif
