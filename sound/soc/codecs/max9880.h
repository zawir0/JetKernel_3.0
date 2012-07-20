/*
 * max9880.h
  *
 * Copyright (C) 2012 KB_JetDroid <kbjetdroid@gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _MAX9880_H
#define _MAX9880_H

/*
 *
 * MAX9880 register space
 *
 */
#define MAX9880_STATUS              0x00
#define MAX9880_JACK_STATUS         0x01
#define MAX9880_AUX_HIGH            0x02
#define MAX9880_AUX_LOW             0x03
#define MAX9880_INT_EN              0x04
#define MAX9880_SYS_CLK             0x05
#define MAX9880_DAI1_CLK_CTL_HIGH   0x06
#define MAX9880_DAI1_CLK_CTL_LOW    0x07
#define MAX9880_DAI1_MODE_A         0x08
#define MAX9880_DAI1_MODE_B         0x09
#define MAX9880_DAI1_TDM            0x0a
#define MAX9880_DAI2_CLK_CTL_HIGH   0x0b
#define MAX9880_DAI2_CLK_CTL_LOW    0x0c
#define MAX9880_DAI2_MODE_A         0x0d
#define MAX9880_DAI2_MODE_B         0x0e
#define MAX9880_DAI2_TDM            0x0f
#define MAX9880_DAC_MIXER           0x10
#define MAX9880_CODEC_FILTER        0x11
#define MAX9880_DSD_CONFIG          0x12
#define MAX9880_DSD_INPUT           0x13
#define MAX9880_REVISION_ID         0x14
#define MAX9880_SIDETONE            0x15
#define MAX9880_STEREO_DAC_LVL      0x16
#define MAX9880_VOICE_DAC_LVL       0x17
#define MAX9880_LEFT_ADC_LVL        0x18
#define MAX9880_RIGHT_ADC_LVL       0x19
#define MAX9880_LEFT_LINE_IN_LVL    0x1a
#define MAX9880_RIGHT_LINE_IN_LVL   0x1b
#define MAX9880_LEFT_VOL_LVL        0x1c
#define MAX9880_RIGHT_VOL_LVL       0x1d
#define MAX9880_LEFT_LINE_OUT_LVL   0x1e
#define MAX9880_RIGHT_LINE_OUT_LVL  0x1f
#define MAX9880_LEFT_MIC_GAIN       0x20
#define MAX9880_RIGHT_MIC_GAIN      0x21
#define MAX9880_INPUT_CONFIG        0x22
#define MAX9880_MIC_CONFIG          0x23
#define MAX9880_MODE_CONFIG         0x24
#define MAX9880_JACK_DETECT         0x25
#define MAX9880_PM_ENABLE           0x26
#define MAX9880_PM_SHUTDOWN         0x27
#define MAX9880_REVISION_ID2         0xff

#define MAX9880_CACHEREGNUM			(MAX9880_PM_SHUTDOWN + 1)

/* Bitfield Definitions */

/* MAX9880_SYS_CLK (0x05) Fields */
#define MAX9880_PSCLK					0x30
#define MAX9880_PSCLK_DIS				(0 << 4)
#define MAX9880_PSCLK_10MHZ_20MHZ		(1 << 4)
#define MAX9880_PSCLK_20MHZ_40MHZ		(2 << 4)
#define MAX9880_PSCLK_40MHZ				(3 << 4)
#define MAX9880_FREQ					0x0f

/* MAX9880_DAI1_CLK_CTL_HIGH (0x06) Fields */
#define MAX9880_PLL1					(1 << 7)
#define MAX9880_NI1_HIGH				0x7f
#define MAX9880_NI1_HIGH_8KHZ			0x10
#define MAX9880_NI1_HIGH_11_025KHZ		0x16
#define MAX9880_NI1_HIGH_12KHZ			0x18
#define MAX9880_NI1_HIGH_16KHZ			0x20
#define MAX9880_NI1_HIGH_22_05KHZ		0x2d
#define MAX9880_NI1_HIGH_24KHZ			0x31
#define MAX9880_NI1_HIGH_32KHZ			0x41
#define MAX9880_NI1_HIGH_44_1KHZ		0x5a
#define MAX9880_NI1_HIGH_48KHZ			0x62

/* MAX9880_DAI1_CLK_CTL_LOW (0x07) Fields */
#define MAX9880_NI1_LOW					0xff
#define MAX9880_NI1_LOW_8KHZ			0x62
#define MAX9880_NI1_LOW_11_025KHZ		0x94
#define MAX9880_NI1_LOW_12KHZ			0x93
#define MAX9880_NI1_LOW_16KHZ			0xc5
#define MAX9880_NI1_LOW_22_05KHZ		0x29
#define MAX9880_NI1_LOW_24KHZ			0x27
#define MAX9880_NI1_LOW_32KHZ			0x89
#define MAX9880_NI1_LOW_44_1KHZ		0x51
#define MAX9880_NI1_LOW_48KHZ			0x4e

/* MAX9880_DAI1_MODE_A (0x08) Fields */
#define MAX9880_MAS1					(1 << 7)
#define MAX9880_WCI1_INVERT				(1 << 6)
#define MAX9880_BCI1					(1 << 5)
#define MAX9880_DLY1					(1 << 4)
#define MAX9880_HIZOFF1					(1 << 3)
#define MAX9880_MTDM1					(1 << 2)
#define MAX9880_FSW1					(1 << 1)
#define MAX9880_WS1						(1 << 0)
#define MAX9880_DIF						0x70
#define MAX9880_DIF_I2S_MODE			0x10

/* MAX9880_DAI1_MODE_B (0x09) Fields */
#define MAX9880_DL1						(1 << 7)
#define MAX9880_SEL1					(1 << 6)
#define MAX9880_SDOEN1					(1 << 5)
#define MAX9880_SDIEN1					(1 << 4)
#define MAX9880_DMONO1					(1 << 3)
#define MAX9880_BSEL1					0x07
#define MAX9880_BSEL1_OFF				(0 << 0)
#define MAX9880_BSEL1_64xLRCLK			(1 << 0)
#define MAX9880_BSEL1_48xLRCLK			(2 << 0)
#define MAX9880_BSEL1_128xLRCLK			(3 << 0)
#define MAX9880_BSEL1_PCLK_2			(4 << 0)
#define MAX9880_BSEL1_PCLK_4			(5 << 0)
#define MAX9880_BSEL1_PCLK_8			(6 << 0)
#define MAX9880_BSEL1_PCLK_16			(7 << 0)

/* MAX9880_DAC_MIXER (0x10) Fields */
#define MAX9880_MIXDAL					0xf0
#define MAX9880_MIXDAL_DAI1_L			(1 << 7)
#define MAX9880_MIXDAL_DAI1_R			(1 << 6)
#define MAX9880_MIXDAL_DAI2_L			(1 << 5)
#define MAX9880_MIXDAL_DAI2_R			(1 << 4)
#define MAX9880_MIXDAR					0x0f
#define MAX9880_MIXDAR_DAI1_L			(1 << 3)
#define MAX9880_MIXDAR_DAI1_R			(1 << 2)
#define MAX9880_MIXDAR_DAI2_L			(1 << 1)
#define MAX9880_MIXDAR_DAI2_R			(1 << 0)

/* MAX9880_CODEC_FILTER (0x11) Fields */
#define MAX9880_MODE_IIR				(0 << 7)
#define MAX9880_MODE_FIR				(1 << 7)
#define MAX9880_AVFLT					0x70
#define MAX9880_AVFLT_DIA1_DCB			(1 << 4)
#define MAX9880_AVFLT_DIA2_DCB			(1 << 3)
#define MAX9880_DVFLT					0x07

/* MAX9880_SIDETONE (0x15) Fields */
#define MAX9880_DSTS					0xc0
#define MAX9880_DSTS_NO_ST				(0 << 6)
#define MAX9880_DSTS_LADC_ST			(1 << 6)
#define MAX9880_DSTS_RADC_ST			(2 << 6)
#define MAX9880_DSTS_LRADC_ST			(3 << 6)
#define MAX9880_DVST					0x1f

/* MAX9880_MODE_CONFIG (0x24) Fields */
#define MAX9880_DLSEW_10MS				(0 << 7)
#define MAX9880_DLSEW_80MS				(1 << 7)
#define MAX9880_VSEN_SLEW				(0 << 6)
#define MAX9880_VSEN_STEP				(1 << 6)
#define MAX9880_ZDEN_EN					(0 << 5)
#define MAX9880_ZDEN_DIS				(1 << 5)
#define MAX9880_DZDEN_EN				(0 << 4)
#define MAX9880_DZDEN_DIS				(1 << 4)
#define MAX9880_HPMODE					0x07
#define MAX9880_HPMODE_STEREO_DIFF		(0 << 0)
#define MAX9880_HPMODE_MONO_DIFF		(1 << 0)
#define MAX9880_HPMODE_STEREO_CAPL		(2 << 0)
#define MAX9880_HPMODE_MONO_CAPL		(3 << 0)
#define MAX9880_HPMODE_STEREO_SE_CL		(4 << 0)
#define MAX9880_HPMODE_MONO_SE_CL		(5 << 0)
#define MAX9880_HPMODE_STEREO_SE_FO		(6 << 0)
#define MAX9880_HPMODE_MONO_SE_FO		(7 << 0)

/* MAX9880_PM_ENABLE (0x26) Fields */
#define MAX9880_LNLEN					(1 << 7)
#define MAX9880_LNREN					(1 << 6)
#define MAX9880_LOLEN					(1 << 5)
#define MAX9880_LOREN					(1 << 4)
#define MAX9880_DALEN					(1 << 3)
#define MAX9880_DAREN					(1 << 2)
#define MAX9880_ADLEN					(1 << 1)
#define MAX9880_ADREN					(1 << 0)

/* MAX9880_PM_SHUTDOWN (0x27) Fields */
#define MAX9880_SHUTDOWN				(1 << 7)
#define MAX9880_XTEN					(1 << 3)
#define MAX9880_XTOSC					(1 << 2)

struct max9880_setup_data {
	unsigned short i2c_address;
};

extern struct snd_soc_dai_driver max9880_dai;
extern struct snd_soc_codec_driver soc_codec_dev_max9880;

#endif
