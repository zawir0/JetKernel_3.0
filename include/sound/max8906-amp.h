/*
 * max8906-amp.h  --  amp driver for max8906
 *
 * Copyright (C) 2012 KB_JetDroid <kbjetdroid@gmail.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef _MAX8906_CODEC_H
#define _MAX8906_CODEC_H

#define MAX8906_PGA_CNTL1			0x84
#define MAX8906_PGA_CNTL2			0x85
#define MAX8906_LMIX_CNTL			0x86
#define MAX8906_RMIX_CNTL			0x87
#define MAX8906_MMIX_CNTL			0x88
#define MAX8906_HS_RIGHT_GAIN_CNTL	0x89
#define MAX8906_HS_LEFT_GAIN_CNTL	0x8A
#define MAX8906_LINE_OUT_GAIN_CNTL	0x8B
#define MAX8906_LS_GAIN_CNTL		0x8C
#define MAX8906_AUDIO_CNTL			0x8D
#define MAX8906_AUDIO_ENABLE1		0x8E
#define MAX8906_CACHEREGNUM		11

/* MAX8906_INPUT_MODE */
#define MAX8906_INB			4
#define MAX8906_INA			5
#define MAX8906_ZCD			6

/* MAX8906_AUDIO_CNTL */
#define MAX8906_HS_MONO_SHIFT		0
#define MAX8906_HS_MONO_SW			(01 << MAX8906_HS_MONO_SHIFT)
#define MAX8906_CLASSD_OSC_SHIFT	2
#define MAX8906_CLASSD_OSC_MASK		(0x3 << MAX8906_CLASSD_OSC_SHIFT)
#define MAX8906_AMP_ENA_SHIFT		4
#define MAX8906_AMP_ENA				(1 << MAX8906_AMP_ENA_SHIFT)
#define MAX8906_SHDN_SHIFT			6
#define MAX8906_SHDN				(1 << MAX8906_SHDN_SHIFT)
#define MAX8906_MUTE_SHIFT			7
#define MAX8906_MUTE				(1 << MAX8906_MUTE_SHIFT)

/* MAX8906_AUDIO_ENABLE1 */
#define MAX8906_HPR_EN_SHIFT		0
#define MAX8906_HPR_EN				(01 << MAX8906_HPR_EN_SHIFT)
#define MAX8906_HPL_EN_SHIFT		1
#define MAX8906_HPL_EN				(0x3 << MAX8906_HPL_EN_SHIFT)
#define MAX8906_LOUT_EN_SHIFT		2
#define MAX8906_LOUT_EN				(1 << MAX8906_LOUT_EN_SHIFT)
#define MAX8906_OUT_EN_SHIFT		3
#define MAX8906_OUT_EN				(1 << MAX8906_OUT_EN_SHIFT)
#define MAX8906_LS_BP_EN_SHIFT		5
#define MAX8906_LS_BP_EN			(1 << MAX8906_LS_BP_EN_SHIFT)

/**
 * struct max8906_platform_data - platform specific MAX8906 configuration
 * @gpio_ampen:	GPIO connected to MAX8906 AMP_EN pin (optional)
 */
struct max8906_codec_platform_data {
	int gpio_ampen;
};

#endif
