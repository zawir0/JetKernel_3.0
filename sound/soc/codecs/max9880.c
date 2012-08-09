/*
 * max9880.c
 * MAX9880 i2s audio codec driver
 *
 * Copyright (C) 2012 KB_JetDroid <kbjetdroid@gmail.com>
 *
 * Created based on ak4671.c by Tomasz Figa <tomasz.figa at gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <linux/slab.h>
#include <linux/gpio.h>
#include <sound/tlv.h>

#include "max9880.h"

#define AUDIO_NAME "max9880"
#define MAX9880_VERSION "0.11" 

#undef MAX9880_DEBUG

#define MAX9880_XTAL 1

#ifdef MAX9880_DEBUG
	#define dbg(format, arg...) \
	printk(KERN_DEBUG AUDIO_NAME ": " format "\n" , ## arg)
#else
	#define dbg(format, arg...) do {} while (0)
#endif

#define err(format, arg...) \
	printk(KERN_ERR AUDIO_NAME ": " format "\n" , ## arg)
	
#define info(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
	
#define warn(format, arg...) \
	printk(KERN_WARNING AUDIO_NAME ": " format "\n" , ## arg)

#undef MAX9880_TRACE

#ifdef MAX9880_TRACE
#define trace(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#else
	#define trace(format, arg...) do {} while (0)
#endif

/* Keep track of device revision ID */
unsigned char revision_id = 0x00;

/* codec private data */
struct max9880_priv {
	enum snd_soc_control_type control_type;
	void *control_data;
};

/*******************************************************************************
 * MAX9880 register cache & default register settings
 ******************************************************************************/
/* max9880 register cache & default register settings */
static const max9880_reg[MAX9880_CACHEREGNUM] = {
	0x00, /* MAX9880_STATUS 			(0x00) */
	0x00, /* MAX9880_JACK_STATUS 		(0x01) */
	0x00, /* MAX9880_AUX_HIGH 			(0x02) */
	0x00, /* MAX9880_AUX_LOW 			(0x03) */
	0x00, /* MAX9880_INT_EN 			(0x04) */
	0x10, /* MAX9880_SYS_CLK 			(0x05) */
	0x80, /* MAX9880_DAI1_CLK_CTL_HIGH 	(0x06) */
	0x00, /* MAX9880_DAI1_CLK_CTL_LOW 	(0x07) */
	0x00, /* MAX9880_DAI1_MODE_A 		(0x08) */
	0x00, /* MAX9880_DAI1_MODE_B 		(0x09) */
	0x00, /* MAX9880_DAI1_TDM 			(0x0a) */
	0x00, /* MAX9880_DAI2_CLK_CTL_HIGH 	(0x0b) */
	0x00, /* MAX9880_DAI2_CLK_CTL_LOW 	(0x0c) */
	0x00, /* MAX9880_DAI2_MODE_A 		(0x0d) */
	0x00, /* MAX9880_DAI2_MODE_B 		(0x0e) */
	0x00, /* MAX9880_DAI2_TDM 			(0x0f) */
	0x00, /* MAX9880_DAC_MIXER 			(0x10) */
	0x00, /* MAX9880_CODEC_FILTER 		(0x11) */
	0x00, /* MAX9880_DSD_CONFIG 		(0x12) */
	0x00, /* MAX9880_DSD_INPUT 			(0x13) */
	0x41, /* MAX9880_REVISION_ID 		(0x14) */
	0x00, /* MAX9880_SIDETONE 			(0x15) */
	0x00, /* MAX9880_STEREO_DAC_LVL 	(0x16) */
	0x00, /* MAX9880_VOICE_DAC_LVL 		(0x17) */
	0x03, /* MAX9880_LEFT_ADC_LVL 		(0x18) */
	0x03, /* MAX9880_RIGHT_ADC_LVL 		(0x19) */
	0x0C, /* MAX9880_LEFT_LINE_IN_LVL 	(0x1a) */
	0x0C, /* MAX9880_RIGHT_LINE_IN_LVL 	(0x1b) */
	0x40, /* MAX9880_LEFT_VOL_LVL 		(0x1c) */
	0x40, /* MAX9880_RIGHT_VOL_LVL 		(0x1d) */
	0x00, /* MAX9880_LEFT_LINE_OUT_LVL 	(0x1e) */
	0x00, /* MAX9880_RIGHT_LINE_OUT_LVL (0x1f) */
	0x51, /* MAX9880_LEFT_MIC_GAIN 		(0x20) */
	0x51, /* MAX9880_RIGHT_MIC_GAIN 	(0x21) */
	0x00, /* MAX9880_INPUT_CONFIG 		(0x22) */
	0x00, /* MAX9880_MIC_CONFIG 		(0x23) */
	0x04, /* MAX9880_MODE_CONFIG 		(0x24) */
	0x00, /* MAX9880_JACK_DETECT 		(0x25) */
	0x00, /* MAX9880_PM_ENABLE 			(0x26) */
	0x00, /* MAX9880_PM_SHUTDOWN 		(0x27) */
};

/*******************************************************************************
 * Get current input mixer setting.
 *
 * Left channel is used to determine current setting.
 ******************************************************************************/
static int max9880_input_mixer_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	trace("%s", __FUNCTION__);

	ucontrol->value.integer.value[0] = snd_soc_read(codec, MAX9880_INPUT_CONFIG) >> 4;

	return 0;
}


/*******************************************************************************
 * Configure input mixer
 *
 * Input mixer mapping
 *  0x01=Main Microphone
 *  0x02=Ear Microphone
 *  0x03=FM In
 *
 ******************************************************************************/
static int max9880_input_mixer_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 reg = snd_soc_read(codec, MAX9880_INPUT_CONFIG);

	trace("%s", __FUNCTION__);

	if ((reg >> 4) == ucontrol->value.integer.value[0])
	{
		return 0;
	}
	else
	{
		// Clear mux bits
		reg &= 0x0f;

		// VALUE index matches MAX9880_INPUT_MIXER enumeration
		switch (ucontrol->value.integer.value[0])
		{
			case 1: // Main Microphone
				reg |= 0x40;
				break;
			case 2: // Ear Microphone
				reg |= 0x10;
				break;
			case 3: // FM In
				reg |= 0xa0;
				break;
			default: // None
				// Leave mux bits cleared
				break;
		}
	}

	snd_soc_write(codec, MAX9880_INPUT_CONFIG, reg);

	return 1;
}

static const char* max9880_filter_mode[] = {"IIR", "FIR"};
static const char* max9880_hp_filter[] = {"None", "1", "2", "3", "4", "5"};
static const char* max9880_sidetone_source[] = {"None", "Left ADC", "Right ADC", "L+R ADC"};

static const char *max9880_input_mixer[] = {"None", "Main Mic", "Ear Mic", "FM In"};
static const struct soc_enum max9880_input_mixer_enum[] = { 
	SOC_ENUM_SINGLE_EXT(4, max9880_input_mixer),
};

static const struct soc_enum max9880_enum[] = {
	// Mask value for SOC_ENUM_SINGLE is the number of elements in the enumeration.
	SOC_ENUM_SINGLE(MAX9880_CODEC_FILTER, 7, 2, max9880_filter_mode),
	SOC_ENUM_SINGLE(MAX9880_CODEC_FILTER, 4, 5, max9880_hp_filter),
	SOC_ENUM_SINGLE(MAX9880_CODEC_FILTER, 0, 5, max9880_hp_filter),
	SOC_ENUM_SINGLE(MAX9880_SIDETONE, 6, 4, max9880_sidetone_source),
};


/*
 * Playback volume control:
 * from -81 to 9 dB in 2.3 dB steps (mute instead of -83 dB)
 */
static DECLARE_TLV_DB_SCALE(master_tlv, -8300, 230, 1);

/*
 * Line Out volume control:
 * from -30 to 0 dB in 2 dB steps (no mute)
 */
static DECLARE_TLV_DB_SCALE(out1_tlv, -3000, 200, 0);

/*
 * Stereo DAC attenuation control:
 * from -15 to 0 dB in 2 dB steps (no mute)
 */
static DECLARE_TLV_DB_SCALE(sdac_tlv, -1500, 100, 0);

/*
 * Voice DAC attenuation control:
 * from -15 to 0 dB in 2 dB steps (no mute)
 */
static DECLARE_TLV_DB_SCALE(vdac_tlv, -1500, 100, 0);

/*
 * Voice DAC Gain control:
 * from 0 to 18 dB in 6 dB steps (no mute)
 */
static DECLARE_TLV_DB_SCALE(dac_tlv, 0, 600, 0);

/*
 * ADC Gain control:
 * from 0 to 18 dB in 6 dB steps (no mute)
 */
static DECLARE_TLV_DB_SCALE(adc_tlv, 0, 600, 0);

/*
 * ADC Level control:
 * from -12 to 3 dB in 1 dB steps (no mute)
 */
static DECLARE_TLV_DB_SCALE(adc_lvl_tlv, -1200, 100, 0);

/*
 * Line In Gain control:
 * from -6 to 24 dB in 2 dB steps (no mute)
 */
static DECLARE_TLV_DB_SCALE(line_in_tlv, -600, 200, 0);

/*
 * Mic Pre-Amp Gain control:
 * from -15 to 30 dB in 15 dB steps (mute instead of -15 dB)
 * -15 db doesn't exist but considered instead of 00=Disabled
 */
static DECLARE_TLV_DB_SCALE(mic_pre_tlv, -1500, 1500, 1);

/*
 * Mic Main-Amp Gain control:
 * from 0 to 20 dB in 1 dB steps (no mute)
 */
static DECLARE_TLV_DB_SCALE(mic_amp_tlv, 0, 100, 0);

static const struct snd_kcontrol_new max9880_snd_controls[] = {

	/* Common playback gain controls */
	// VOLL and VOLR
	SOC_DOUBLE_R_TLV("Master Playback Volume", MAX9880_LEFT_VOL_LVL, MAX9880_RIGHT_VOL_LVL, 0, 0x20, 1, master_tlv),
	// Line Output Gain
	SOC_DOUBLE_R_TLV("Line Output Playback Volume", MAX9880_LEFT_LINE_OUT_LVL, MAX9880_RIGHT_LINE_OUT_LVL, 0, 0x0f, 1, out1_tlv),
	// SDACA Attenuation
	SOC_SINGLE_TLV("SDACA Attenuation", MAX9880_STEREO_DAC_LVL, 0, 0x0f, 1, sdac_tlv),
	// VDACA Attenuation
	SOC_SINGLE_TLV("VDACA Attenuation", MAX9880_VOICE_DAC_LVL, 0, 0x0f, 1, vdac_tlv),
	// DAC Gain
	SOC_SINGLE_TLV("DAC Gain", MAX9880_VOICE_DAC_LVL, 4, 0x03, 0, dac_tlv),

	/* Common capture gain controls */
	// ADC Gain
	SOC_DOUBLE_R_TLV("ADC Gain", MAX9880_LEFT_ADC_LVL, MAX9880_RIGHT_ADC_LVL, 4, 0x03, 0, adc_tlv),
	// ADC Level
	SOC_DOUBLE_R_TLV("ADC Level", MAX9880_LEFT_ADC_LVL, MAX9880_RIGHT_ADC_LVL, 0, 0x0f, 1, adc_lvl_tlv),
	// Line Input Gain
	SOC_DOUBLE_R_TLV("Line Input Gain", MAX9880_LEFT_LINE_IN_LVL, MAX9880_RIGHT_LINE_IN_LVL, 0, 0x0f, 1, line_in_tlv),
	// Microphone Pre-Amp
	SOC_DOUBLE_R_TLV("Microphone Pre-Amp", MAX9880_LEFT_MIC_GAIN, MAX9880_RIGHT_MIC_GAIN, 5, 0x03, 0, mic_pre_tlv),
	// Microphone PGA
	SOC_DOUBLE_R_TLV("Microphone PGA", MAX9880_LEFT_MIC_GAIN, MAX9880_RIGHT_MIC_GAIN, 0, 0x14, 1, mic_amp_tlv),
	// Input Mixer


	// CODEC Filter MODE
	SOC_ENUM("Filter Mode", max9880_enum[0]),
	// ADC Filter
	SOC_ENUM("ADC Filter", max9880_enum[1]),
	// DAC Filter
	SOC_ENUM("DAC Filter", max9880_enum[2]),
	// Sidetone Source
	SOC_ENUM("Sidetone Source", max9880_enum[3]),
	// Sidetone level
	SOC_SINGLE("Sidetone Level", MAX9880_SIDETONE, 0, 31, 1),
	// Input Mixer
  SOC_ENUM_EXT("Input Mixer", max9880_input_mixer_enum , max9880_input_mixer_get, max9880_input_mixer_set),
};

/* Output Mixer */
static const struct snd_kcontrol_new max9880_left_output_mixer_controls[] = {
		SOC_DAPM_SINGLE("Left LIN", MAX9880_PM_ENABLE, 7, 1, 0),
		SOC_DAPM_SINGLE("DACL", MAX9880_PM_ENABLE, 3, 1, 0),
};

static const struct snd_kcontrol_new max9880_right_output_mixer_controls[] = {
		SOC_DAPM_SINGLE("Right LIN", MAX9880_PM_ENABLE, 6, 1, 0),
		SOC_DAPM_SINGLE("DACR", MAX9880_PM_ENABLE, 2, 1, 0),
};

static const struct snd_soc_dapm_widget max9880_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MMICIN"),  // Analog microphone input
	SND_SOC_DAPM_INPUT("EMICIN"),  // Analog microphone input
	SND_SOC_DAPM_INPUT("LLINEIN"), // Line input
	SND_SOC_DAPM_INPUT("RLINEIN"), // Line input

	SND_SOC_DAPM_OUTPUT("LOUTP"), // Speaker output
	SND_SOC_DAPM_OUTPUT("ROUTP"), // Speaker output
	SND_SOC_DAPM_OUTPUT("LOUTL"), // Line output
	SND_SOC_DAPM_OUTPUT("LOUTR"), // Line output

	SND_SOC_DAPM_ADC("ADC Left", "Left HiFi Capture", MAX9880_PM_ENABLE, 1, 0),
	SND_SOC_DAPM_ADC("ADC Right", "Right HiFi Capture", MAX9880_PM_ENABLE, 1, 0),

	SND_SOC_DAPM_MIXER("Input Mixer", SND_SOC_NOPM, 0, 0, NULL, 0), 

	SND_SOC_DAPM_MIXER("LOUT Mixer", MAX9880_PM_ENABLE, 5, 0, &max9880_left_output_mixer_controls[0],
		ARRAY_SIZE(max9880_left_output_mixer_controls)),

	SND_SOC_DAPM_MIXER("ROUT Mixer", MAX9880_PM_ENABLE, 4, 0, &max9880_right_output_mixer_controls[0],
		ARRAY_SIZE(max9880_right_output_mixer_controls)),

	SND_SOC_DAPM_DAC("DAC Left", "Left HiFi Playback", MAX9880_PM_ENABLE, 3, 0),
	SND_SOC_DAPM_DAC("DAC Right", "Right HiFi Playback", MAX9880_PM_ENABLE, 2, 0),

};

static const struct snd_soc_dapm_route max9880_intercon[] = {
	/* Inputs */
	//{"Mic Input", NULL, "LMICIN"},
	//{"Mic Input", NULL, "RMICIN"},
	//{"Line Input", NULL, "LLINEIN"},
	//{"Line Input", NULL, "RLINEIN"},
	{"Input Mixer", NULL, "MMICIN"},
	{"Input Mixer", NULL, "EMICIN"},
	/* Outputs */
	{"LOUTP", "NULL", "LOUT Mixer"}, // Speaker output
	{"ROUTP", "NULL", "ROUT Mixer"}, // Speaker output
	{"LOUTL", "NULL", "LOUT Mixer"}, // Line output
	{"LOUTR", "NULL", "ROUT Mixer"}, // Line output

	/* Input mixer */
	//{"Input Mixer", NULL, "Line Input"},
	//{"Input Mixer", NULL, "Mic Input"},

	/* ADC */
	{"ADC Left", "NULL", "Input Mixer"},
	{"ADC Right", "NULL", "Input Mixer"},

	/* Output mixer */
	{"LOUT Mixer", "Left LIN", "LLINEIN"},
	{"ROUT Mixer", "Right LIN", "RLINEIN"},

	//{"Output Mixer", NULL, "Line Input"},
	{"LOUT Mixer", "DACL", "DAC Left"},
	{"ROUT Mixer", "DACR", "DAC Right"},

	/* TERMINATOR */
	//{NULL, NULL, NULL},
};

static int max9880_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 fs_high, fs_low, reg;

	fs_high = snd_soc_read(codec, MAX9880_DAI1_CLK_CTL_HIGH);
	fs_low = snd_soc_read(codec, MAX9880_DAI1_CLK_CTL_LOW);
	fs_high &= ~MAX9880_NI1_HIGH;
	fs_low &= ~MAX9880_NI1_LOW;

	switch (params_rate(params)) {
	case 8000:
		fs_high |= MAX9880_NI1_HIGH_8KHZ;
		fs_low |= MAX9880_NI1_LOW_8KHZ;
		break;
	case 11025:
		fs_high |= MAX9880_NI1_HIGH_11_025KHZ;
		fs_low |= MAX9880_NI1_LOW_11_025KHZ;
		break;
	case 12000:
		fs_high |= MAX9880_NI1_HIGH_12KHZ;
		fs_low |= MAX9880_NI1_LOW_12KHZ;
		break;
	case 16000:
		fs_high |= MAX9880_NI1_HIGH_16KHZ;
		fs_low |= MAX9880_NI1_LOW_16KHZ;
		break;
	case 22050:
		fs_high |= MAX9880_NI1_HIGH_22_05KHZ;
		fs_low |= MAX9880_NI1_LOW_22_05KHZ;
		break;
	case 24000:
		fs_high |= MAX9880_NI1_HIGH_24KHZ;
		fs_low |= MAX9880_NI1_LOW_24KHZ;
		break;
	case 32000:
		fs_high |= MAX9880_NI1_HIGH_32KHZ;
		fs_low |= MAX9880_NI1_LOW_32KHZ;
		break;
	case 44100:
		fs_high |= MAX9880_NI1_HIGH_44_1KHZ;
		fs_low |= MAX9880_NI1_LOW_44_1KHZ;
		break;
	case 48000:
		fs_high |= MAX9880_NI1_HIGH_48KHZ;
		fs_low |= MAX9880_NI1_LOW_48KHZ;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, MAX9880_DAI1_CLK_CTL_HIGH, fs_high);
	snd_soc_write(codec, MAX9880_DAI1_CLK_CTL_LOW, fs_low);

// Misc register setting
// TODO: Move these register settings to appropriate widgets if possible

	reg = snd_soc_read(codec, MAX9880_DAI1_MODE_B);
	reg &= ~MAX9880_BSEL1;
	reg |= MAX9880_SDOEN1;
	reg |= MAX9880_SDIEN1;
	reg |= MAX9880_BSEL1_48xLRCLK;
	snd_soc_write(codec, MAX9880_DAI1_MODE_B, reg);

#if 1
	snd_soc_write(codec, MAX9880_DAI1_TDM,
						(0 << 6) | // STOTL
						(1 << 4) | // STOTR
						(0 << 0)); // STOTDLY
#endif

	reg = snd_soc_read(codec, MAX9880_DAC_MIXER);
	reg &= ~MAX9880_MIXDAL;
	reg |= MAX9880_MIXDAL_DAI1_L;
	reg &= ~MAX9880_MIXDAR;
	reg |= MAX9880_MIXDAR_DAI1_R;
	snd_soc_write(codec, MAX9880_DAC_MIXER, reg);

	// voice/audio filter
	reg = snd_soc_read(codec, MAX9880_CODEC_FILTER);
	reg &= ~(0xff);
	reg |= MAX9880_MODE_FIR;
	snd_soc_write(codec, MAX9880_CODEC_FILTER, reg);


	// side tone off
	reg = snd_soc_read(codec, MAX9880_SIDETONE);
	reg &= ~(0xff);
	reg |= MAX9880_DSTS_NO_ST;
	snd_soc_write(codec, MAX9880_SIDETONE, reg);

	// DAC levels
	snd_soc_write(codec, MAX9880_STEREO_DAC_LVL, 0x00);
	snd_soc_write(codec, MAX9880_VOICE_DAC_LVL, 0x00);

	// line in
	snd_soc_write(codec, MAX9880_LEFT_LINE_IN_LVL, 0x00);
	snd_soc_write(codec, MAX9880_RIGHT_LINE_IN_LVL, 0x00);

	// DAC volume
	snd_soc_write(codec, MAX9880_LEFT_VOL_LVL, 0x0C); //0x0d);
	snd_soc_write(codec, MAX9880_RIGHT_VOL_LVL, 0x0C); //0x0d);

	// line out
	snd_soc_write(codec, MAX9880_LEFT_LINE_OUT_LVL, 0x00); //0x0f);
	snd_soc_write(codec, MAX9880_RIGHT_LINE_OUT_LVL, 0x00); //0x0f);

	// ADC gain
	snd_soc_write(codec, MAX9880_LEFT_ADC_LVL, 0x09);
	snd_soc_write(codec, MAX9880_RIGHT_ADC_LVL, 0x09);

	// mode
	reg = snd_soc_read(codec, MAX9880_MODE_CONFIG);
	reg |= MAX9880_DLSEW_10MS;
	reg |= MAX9880_VSEN_SLEW;
	reg |= MAX9880_DZDEN_DIS;
	reg &= ~MAX9880_HPMODE;
	reg |= MAX9880_HPMODE_STEREO_CAPL;
	snd_soc_write(codec, MAX9880_MODE_CONFIG, reg);

	return 0;
}

static int max9880_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 pll, power;

	pll = snd_soc_read(codec, MAX9880_SYS_CLK);
	power = snd_soc_read(codec, MAX9880_PM_SHUTDOWN);
	pll &= ~MAX9880_PSCLK;

#ifdef MAX9880_XTAL
	pll &= ~MAX9880_FREQ;
	power |= MAX9880_XTEN;
#else
	//Not required for Jet
#endif
	if (freq >= 10000000 && freq <= 20000000)
		pll |= MAX9880_PSCLK_10MHZ_20MHZ;
	else if (freq >= 20000000 && freq <= 40000000)
		pll |= MAX9880_PSCLK_20MHZ_40MHZ;
	else if (freq >= 40000000)
		pll |= MAX9880_PSCLK_40MHZ;
	else
		pll |= MAX9880_PSCLK_DIS;

	snd_soc_write(codec, MAX9880_SYS_CLK, pll);
	snd_soc_write(codec, MAX9880_PM_SHUTDOWN, power);

	return 0;
}

static int max9880_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 mode;
	u8 pll;

	/* set master/slave audio interface */
	mode = snd_soc_read(codec, MAX9880_DAI1_MODE_A);
	pll = snd_soc_read(codec, MAX9880_SYS_CLK);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		mode |= MAX9880_MAS1;
		pll &= ~MAX9880_MAS1;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		mode |= MAX9880_MAS1;
		pll |= MAX9880_PLL1;
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	mode &= ~MAX9880_DIF;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		mode |= MAX9880_DIF_I2S_MODE;
		break;
	default:
		return -EINVAL;
	}

	/* set mode and format */
	snd_soc_write(codec, MAX9880_DAI1_MODE_A, mode);
	snd_soc_write(codec, MAX9880_SYS_CLK, pll);

	return 0;
}

static int max9880_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	int ret;

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		ret = snd_soc_cache_sync(codec);
		if (ret) {
			dev_err(codec->dev,
				"Failed to sync cache: %d\n", ret);
			return ret;
		}
		snd_soc_update_bits(codec, MAX9880_PM_SHUTDOWN,
				MAX9880_SHUTDOWN, 0x80);
		break;
	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec, MAX9880_PM_SHUTDOWN,
				MAX9880_SHUTDOWN, 0x00);
		codec->cache_sync = 1;
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static int max9880_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	int ret;

	ret = max9880_set_bias_level(codec, SND_SOC_BIAS_OFF);

	if (ret < 0)
		return ret;

	return 0;
}

/*******************************************************************************
 * Volume control mute
 ******************************************************************************/
static int max9880_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	u8 muteL = snd_soc_read(codec, MAX9880_LEFT_VOL_LVL) & 0x3f;
	u8 muteR = snd_soc_read(codec, MAX9880_RIGHT_VOL_LVL) & 0x3f;

	trace("%s %s", __FUNCTION__, (mute) ? "muted" : "unmuted");

	if (mute)
	{
		muteL |= 0x40;
		muteR |= 0x40;
	}

	snd_soc_write(codec, MAX9880_LEFT_VOL_LVL, muteL);
	snd_soc_write(codec, MAX9880_RIGHT_VOL_LVL, muteR);

	return 0;
}

static int max9880_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	max9880_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int max9880_resume(struct snd_soc_codec *codec)
{
	max9880_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	return 0;
}

static struct snd_soc_dai_ops max9880_dai_ops = {
	.digital_mute = max9880_mute,
	.hw_params	= max9880_hw_params,
	.set_sysclk	= max9880_set_dai_sysclk,
	.set_fmt	= max9880_set_dai_fmt,
	.hw_free	= max9880_hw_free,
};

#define MAX9880_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
                       SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
                       SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
                       SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |\
                       SNDRV_PCM_RATE_96000)

#define MAX9880_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)

struct snd_soc_dai_driver max9880_dai = {
	.name = "max9880-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = MAX9880_RATES,
		.formats = MAX9880_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = MAX9880_RATES,
		.formats = MAX9880_FORMATS,},
	.ops = &max9880_dai_ops
};
//EXPORT_SYMBOL_GPL(max9880_dai);

/*******************************************************************************
 * Make sure that a MAX9880 is attached to I2C bus.
 ******************************************************************************/
static int max9880_probe(struct snd_soc_codec *codec)
{
	struct max9880_priv *max9880 = snd_soc_codec_get_drvdata(codec);
	int ret;

	ret = snd_soc_codec_set_cache_io(codec, 8, 8, max9880->control_type);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	max9880_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

static int max9880_remove(struct snd_soc_codec *codec)
{
	max9880_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

struct snd_soc_codec_driver soc_codec_dev_max9880 = {
	.probe = max9880_probe,
	.remove = max9880_remove,
	.suspend = max9880_suspend,
	.resume = max9880_resume,
	.set_bias_level = max9880_set_bias_level,
	.reg_cache_size = MAX9880_CACHEREGNUM,
	.reg_word_size = sizeof(u8),
	.reg_cache_default = max9880_reg,
	.controls = max9880_snd_controls,
	.num_controls = ARRAY_SIZE(max9880_snd_controls),
	.dapm_widgets = max9880_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(max9880_dapm_widgets),
	.dapm_routes = max9880_intercon,
	.num_dapm_routes = ARRAY_SIZE(max9880_intercon),
};

//EXPORT_SYMBOL_GPL(soc_codec_dev_max9880);

/*******************************************************************************
 * I2C codec control layer
 ******************************************************************************/
static int __devinit max9880_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct max9880_priv *max9880;
	int ret;


	max9880 = kzalloc(sizeof(struct max9880_priv), GFP_KERNEL);
	if (max9880 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(client, max9880);
	max9880->control_data = client;
	max9880->control_type = SND_SOC_I2C;

	ret = snd_soc_register_codec(&client->dev,
			&soc_codec_dev_max9880, &max9880_dai, 1);
	if (ret < 0)
		goto err;

	return ret;
err:
	kfree(max9880);
	return ret;
}

static __devexit int max9880_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id max9880_i2c_id[] = {
	{ "max9880", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max9880_i2c_id);

static struct i2c_driver max9880_i2c_driver = {
	.driver = {
		.name = "max9880-codec",
		.owner = THIS_MODULE,
	},
	.probe = max9880_i2c_probe,
	.remove = __devexit_p(max9880_i2c_remove),
	.id_table = max9880_i2c_id,
};

static int __init max9880_modinit(void)
{
	return i2c_add_driver(&max9880_i2c_driver);
}
module_init(max9880_modinit);

static void __exit max9880_exit(void)
{
	i2c_del_driver(&max9880_i2c_driver);
}
module_exit(max9880_exit);

MODULE_DESCRIPTION("ASoC MAX9880 driver");
MODULE_AUTHOR("KB_JetDroid");
MODULE_LICENSE("GPL");
