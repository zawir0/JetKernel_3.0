/*
 * max8906-amp.c
 * MAX8906 dailess amplifier driver
 *
 * Copyright (C) 2012 KB_JetDroid <kbjetdroid@gmail.com>
 *
 * Created based on max9877.c by Tomasz Figa <tomasz.figa at gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/soc-dapm.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <sound/max8906-amp.h>

extern unsigned int max8906_audio_i2c_read (u8 reg);
extern int max8906_audio_i2c_write (u8 reg, u8 data);

/* codec private data */
struct max8906_priv {
	enum snd_soc_control_type control_type;
	void *control_data;
	unsigned int mix_power;
	unsigned int mix_mode;
	struct mutex mix_mutex;
};

/* default register values */
static u8 max8906_regs[MAX8906_CACHEREGNUM] = {
	0x00, /* MAX8906_PGA_CNTL1			(0x84) */
	0x22, /* MAX8906_PGA_CNTL2			(0x85) */
	0x02, /* MAX8906_LMIX_CNTL			(0x86) */
	0x01, /* MAX8906_RMIX_CNTL			(0x87) */
	0x00, /* MAX8906_MMIX_CNTL			(0x88) */
	0x10, /* MAX8906_HS_RIGHT_GAIN_CNTL	(0x89) */
	0x10, /* MAX8906_HS_LEFT_GAIN_CNTL	(0x8A) */
	0x00, /* MAX8906_LINE_OUT_GAIN_CNTL	(0x8B) */
	0x1B, /* MAX8906_LS_GAIN_CNTL		(0x8C) */
	0x00, /* MAX8906_AUDIO_CNTL			(0x8D) */
	0x00, /* MAX8906_AUDIO_ENABLE1		(0x8E) */
};

/*
 * Amplifier Gain controls:
 * FIXME: DB range is not known. Don't use this
 */
static const unsigned int max8906_pgain_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 900, 0),
	2, 2, TLV_DB_SCALE_ITEM(2000, 0, 0),
};

static const unsigned int max8906_pgaout_tlv[] = {
	TLV_DB_RANGE_HEAD(4),
	0, 7, TLV_DB_SCALE_ITEM(-7900, 400, 1),
	8, 15, TLV_DB_SCALE_ITEM(-4700, 300, 0),
	16, 23, TLV_DB_SCALE_ITEM(-2300, 200, 0),
	24, 31, TLV_DB_SCALE_ITEM(-700, 100, 0),
};

/*
 * Class D Amplifier Osc control:
 * FIXME: Don't know the values. Don't use this
 */

static const char *max8906_osc_mode[] = {
	"1176KHz",
	"1100KHz",
	"700KHz",
};
SOC_ENUM_SINGLE_DECL(max8906_osc_mode_enum, MAX8906_AUDIO_CNTL,
		MAX8906_CLASSD_OSC_SHIFT, max8906_osc_mode);

static const struct snd_kcontrol_new max8906_controls[] = {
	SOC_SINGLE_TLV("MAX8906 PGAVOICEP Playback Volume",
			MAX8906_PGA_CNTL1, 6, 3, 0, max8906_pgain_tlv),
	SOC_SINGLE_TLV("MAX8906 PGAVOICEN Playback Volume",
			MAX8906_PGA_CNTL1, 4, 3, 0, max8906_pgain_tlv),
	SOC_SINGLE_TLV("MAX8906 PGAIN1 Playback Volume",
			MAX8906_PGA_CNTL1, 0, 3, 0, max8906_pgain_tlv),
	SOC_SINGLE_TLV("MAX8906 PGAIN2 Playback Volume",
			MAX8906_PGA_CNTL2, 6, 3, 0, max8906_pgain_tlv),
	SOC_SINGLE_TLV("MAX8906 PGAIN3 Playback Volume",
			MAX8906_PGA_CNTL2, 4, 3, 0, max8906_pgain_tlv),
	SOC_SINGLE_TLV("MAX8906 PGAIN4 Playback Volume",
			MAX8906_PGA_CNTL2, 1, 5, 0, max8906_pgain_tlv),


	SOC_SINGLE_TLV("MAX8906 HPR Playback Volume",
			MAX8906_HS_RIGHT_GAIN_CNTL, 0, 0x1F, 0, max8906_pgaout_tlv),
	SOC_SINGLE_TLV("MAX8906 HPL Playback Volume",
			MAX8906_HS_LEFT_GAIN_CNTL, 0, 0x1F, 0, max8906_pgaout_tlv),
	SOC_SINGLE_TLV("MAX8906 LOUT Playback Volume",
			MAX8906_HS_LEFT_GAIN_CNTL, 0, 0x1F, 0, max8906_pgaout_tlv),
	SOC_SINGLE_TLV("MAX8906 OUT Playback Volume",
			MAX8906_LS_GAIN_CNTL, 0, 0x1F, 0, max8906_pgaout_tlv),

	SOC_SINGLE("MAX8906 Zero-crossing detection Switch",
			MAX8906_PGA_CNTL2, 0, 1, 0),
	SOC_ENUM("MAX8906 Oscillator Mode", max8906_osc_mode_enum),
};

/* Output Amps */
#define OUT_POWER		(1 << 3)
#define LOUT_POWER		(1 << 2)
#define HPL_POWER		(1 << 1)
#define HPR_POWER		(1 << 0)

unsigned int max8906_codec_read (struct snd_soc_codec *codec, unsigned int reg)
{
	return max8906_audio_i2c_read(reg);
}
int max8906_codec_write (struct snd_soc_codec *codec, unsigned int reg, unsigned int data)
{
	return max8906_audio_i2c_write(reg, data);
}
/* Called with mux_mutex held. */
static void max8906_update_mux(struct snd_soc_codec *codec)
{
	struct max8906_priv *max8906 = snd_soc_codec_get_drvdata(codec);
	unsigned int power = max8906->mix_power;
	u8 reg;

	printk("%s: mode = %u, power = %u\n", __func__,
					max8906->mix_mode, max8906->mix_power);

	if (!power) {
		reg = snd_soc_read(codec, MAX8906_AUDIO_CNTL);
		reg &= ~MAX8906_SHDN;
		snd_soc_write(codec, MAX8906_AUDIO_CNTL, reg);
		return;
	}

	reg = snd_soc_read(codec, MAX8906_AUDIO_CNTL);
	reg |= MAX8906_SHDN;
	snd_soc_write(codec, MAX8906_AUDIO_CNTL, reg);
}

static int max8906_out_amp_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct max8906_priv *max8906 = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&max8906->mix_mutex);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:
		max8906->mix_power &= ~OUT_POWER;
		max8906_update_mux(codec);
		break;
	case SND_SOC_DAPM_POST_PMU:
		max8906->mix_power |= OUT_POWER;
		max8906_update_mux(codec);
		break;
	}

	mutex_unlock(&max8906->mix_mutex);

	return 0;
}

static int max8906_lout_amp_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct max8906_priv *max8906 = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&max8906->mix_mutex);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:
		max8906->mix_power &= ~LOUT_POWER;
		max8906_update_mux(codec);
		break;
	case SND_SOC_DAPM_POST_PMU:
		max8906->mix_power |= LOUT_POWER;
		max8906_update_mux(codec);
		break;
	}

	mutex_unlock(&max8906->mix_mutex);

	return 0;
}

static int max8906_hpl_amp_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct max8906_priv *max8906 = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&max8906->mix_mutex);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:
		max8906->mix_power &= ~HPL_POWER;
		max8906_update_mux(codec);
		break;
	case SND_SOC_DAPM_POST_PMU:
		max8906->mix_power |= HPL_POWER;
		max8906_update_mux(codec);
		break;
	}

	mutex_unlock(&max8906->mix_mutex);

	return 0;
}

static int max8906_hpr_amp_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct max8906_priv *max8906 = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&max8906->mix_mutex);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:
		max8906->mix_power &= ~HPR_POWER;
		max8906_update_mux(codec);
		break;
	case SND_SOC_DAPM_POST_PMU:
		max8906->mix_power |= HPR_POWER;
		max8906_update_mux(codec);
		break;
	}

	mutex_unlock(&max8906->mix_mutex);

	return 0;
}

/* OUT bypass */

static const char *max8906_out_bypass_texts[] = {"OUT Amp", "SPKBPIN"};
SOC_ENUM_SINGLE_DECL(max8906_out_bypass_enum, MAX8906_AUDIO_ENABLE1,
		MAX8906_LS_BP_EN_SHIFT, max8906_out_bypass_texts);
static const struct snd_kcontrol_new max8906_out_bypass_control =
		SOC_DAPM_ENUM("Route", max8906_out_bypass_enum);

/* Input MUXs */
static const struct snd_kcontrol_new max8906_lin_mixer_controls[] = {
	SOC_DAPM_SINGLE("LVOICEINP", MAX8906_LMIX_CNTL, 5, 1, 0),
	SOC_DAPM_SINGLE("LVOICEINN", MAX8906_LMIX_CNTL, 4, 1, 0),
	SOC_DAPM_SINGLE("LIN1", MAX8906_LMIX_CNTL, 3, 1, 0),
	SOC_DAPM_SINGLE("LIN2", MAX8906_LMIX_CNTL, 2, 1, 0),
	SOC_DAPM_SINGLE("LIN3", MAX8906_LMIX_CNTL, 1, 1, 0),
	SOC_DAPM_SINGLE("LIN4", MAX8906_LMIX_CNTL, 0, 1, 0),
};

static const struct snd_kcontrol_new max8906_rin_mixer_controls[] = {
	SOC_DAPM_SINGLE("RVOICEINP", MAX8906_RMIX_CNTL, 5, 1, 0),
	SOC_DAPM_SINGLE("RVOICEINN", MAX8906_RMIX_CNTL, 4, 1, 0),
	SOC_DAPM_SINGLE("RIN1", MAX8906_RMIX_CNTL, 3, 1, 0),
	SOC_DAPM_SINGLE("RIN2", MAX8906_RMIX_CNTL, 2, 1, 0),
	SOC_DAPM_SINGLE("RIN3", MAX8906_RMIX_CNTL, 1, 1, 0),
	SOC_DAPM_SINGLE("RIN4", MAX8906_RMIX_CNTL, 0, 1, 0),
};

/* DAPM widgets */

static const struct snd_soc_dapm_widget max8906_dapm_widgets[] = {
	/* INPUTS */
	SND_SOC_DAPM_INPUT("VOICEINP"),
	SND_SOC_DAPM_INPUT("VOICEINN"),
	SND_SOC_DAPM_INPUT("IN1"),
	SND_SOC_DAPM_INPUT("IN2"),
	SND_SOC_DAPM_INPUT("IN3"),
	SND_SOC_DAPM_INPUT("IN4"),
	SND_SOC_DAPM_INPUT("SPKBPIN"),

	/* OUTPUTS */
	SND_SOC_DAPM_OUTPUT("HPL"),
	SND_SOC_DAPM_OUTPUT("HPR"),
	SND_SOC_DAPM_OUTPUT("OUT"),
	SND_SOC_DAPM_OUTPUT("LOUT"),

	/* MUXES */
	SND_SOC_DAPM_MUX("OUT Bypass", SND_SOC_NOPM, 0, 0,
				&max8906_out_bypass_control),

	/* Input Mixers */
	SND_SOC_DAPM_MIXER("LIN Mixer", SND_SOC_NOPM, 0, 0,
			&max8906_lin_mixer_controls[0],
			ARRAY_SIZE(max8906_lin_mixer_controls)),
	SND_SOC_DAPM_MIXER("RIN Mixer", SND_SOC_NOPM, 0, 0,
			&max8906_rin_mixer_controls[0],
			ARRAY_SIZE(max8906_rin_mixer_controls)),

	/* In PGAs */
	SND_SOC_DAPM_PGA("VOICEINP PGA",
			SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("VOICEINN PGA",
			SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IN1 PGA",
			SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IN2 PGA",
			SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IN3 PGA",
			SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("IN4 PGA",
			SND_SOC_NOPM, 0, 0, NULL, 0),

	/* Out PGAs */
	SND_SOC_DAPM_PGA_E("OUT Amp", MAX8906_AUDIO_ENABLE1, 3, 0, NULL, 0,
			max8906_out_amp_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_PGA_E("LOUT Amp", MAX8906_AUDIO_ENABLE1, 2, 0, NULL, 0,
			max8906_lout_amp_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_PGA_E("HPL Amp", MAX8906_AUDIO_ENABLE1, 1, 0, NULL, 0,
			max8906_hpl_amp_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_PGA_E("HPR Amp", MAX8906_AUDIO_ENABLE1, 0, 0, NULL, 0,
			max8906_hpr_amp_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

};

static const struct snd_soc_dapm_route max8906_dapm_routes[] = {
	{"VOICEINP PGA", NULL, "VOICEINP"},
	{"VOICEINN PGA", NULL, "VOICEINN"},
	{"IN1 PGA", NULL, "IN1"},
	{"IN2 PGA", NULL, "IN2"},
	{"IN3 PGA", NULL, "IN3"},
	{"IN4 PGA", NULL, "IN4"},

	/* Inputs */
	{"LIN Mixer", "LVOICEINP", "VOICEINP PGA"},
	{"LIN Mixer", "LVOICEINN", "VOICEINN PGA"},
	{"LIN Mixer", "LIN1", "IN1 PGA"},
	{"LIN Mixer", "LIN2", "IN2 PGA"},
	{"LIN Mixer", "LIN3", "IN3 PGA"},
	{"LIN Mixer", "LIN4", "IN4 PGA"},

	{"RIN Mixer", "RVOICEINP", "VOICEINP PGA"},
	{"RIN Mixer", "RVOICEINN", "VOICEINN PGA"},
	{"RIN Mixer", "RIN1", "IN1 PGA"},
	{"RIN Mixer", "RIN2", "IN2 PGA"},
	{"RIN Mixer", "RIN3", "IN3 PGA"},
	{"RIN Mixer", "RIN4", "IN4 PGA"},

	{"OUT Amp", NULL, "LIN Mixer"},
	{"OUT Amp", NULL, "RIN Mixer"},
	{"HPL Amp", NULL, "LIN Mixer"},
	{"HPR Amp", NULL, "RIN Mixer"},

	{"OUT Bypass", "OUT Amp", "OUT Amp"},
	{"OUT Bypass", "SPKBPIN", "SPKBPIN"},

	{"HPL", NULL, "HPL Amp"},
	{"HPR", NULL, "HPR Amp"},
	{"OUT", NULL, "OUT Bypass"},
};

static int max8906_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	struct max8906_codec_platform_data *pdata = codec->dev->platform_data;

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {
			if (pdata && gpio_is_valid(pdata->gpio_ampen))
			{
				gpio_set_value(pdata->gpio_ampen, 1);
				printk("KB: Enabling 8906 AMP\n");
				snd_soc_write(codec, 0x8E, 0x08);
			}
		}
		break;
	case SND_SOC_BIAS_OFF:
		snd_soc_write(codec, MAX8906_AUDIO_ENABLE1, 0x00);
		if (pdata && gpio_is_valid(pdata->gpio_ampen))
		{
			gpio_set_value(pdata->gpio_ampen, 0);
			printk("KB: Disabling 8906 AMP\n");
			snd_soc_write(codec, 0x8E, 0x00);
		}
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static int max8906_probe(struct snd_soc_codec *codec)
{
	int ret;
	int i;
	struct max8906_codec_platform_data *pdata = codec->dev->platform_data;

	if (pdata) {
		if (gpio_is_valid(pdata->gpio_ampen)) {
			ret = gpio_request_one(pdata->gpio_ampen,
					GPIOF_OUT_INIT_LOW, "max8906 ampen");
			if (ret)
				return ret;

			udelay(1); /* > 150 ns */
			gpio_set_value(pdata->gpio_ampen, 1);
			printk("KB: Enabling MAX8906 AMP\n");
		}
	}

#if 0
	ret = snd_soc_codec_set_cache_io(codec, 8, 8, max8906->control_type);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}
#endif

	codec->read = max8906_codec_read;
	codec->write = max8906_codec_write;

	snd_soc_write(codec, 0x8D, 0x00);

	for (i = 0; i < ARRAY_SIZE(max8906_regs); ++i)
		snd_soc_write(codec, i + 0x84, max8906_regs[i]);

	return ret;
}

static int max8906_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_max8906 = {
	.probe = max8906_probe,
	.remove = max8906_remove,
	.set_bias_level = max8906_set_bias_level,
//	.reg_cache_size = MAX8906_CACHEREGNUM,
//	.reg_word_size = sizeof(u8),
//	.reg_cache_default = max8906_regs,
	.controls = max8906_controls,
	.num_controls = ARRAY_SIZE(max8906_controls),
	.dapm_widgets = max8906_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(max8906_dapm_widgets),
	.dapm_routes = max8906_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(max8906_dapm_routes),
};

static int __devinit max8906_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct max8906_priv *max8906;
	int ret;

	max8906 = kzalloc(sizeof(struct max8906_priv), GFP_KERNEL);
	if (max8906 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(client, max8906);
	max8906->control_data = client;
	max8906->control_type = SND_SOC_I2C;
	max8906->mix_power = 0;
	max8906->mix_mode = 0;
	mutex_init(&max8906->mix_mutex);

	ret = snd_soc_register_codec(&client->dev,
					&soc_codec_dev_max8906, NULL, 0);
	if (ret < 0)
		kfree(max8906);
	return ret;
}

static __devexit int max8906_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id max8906_i2c_id[] = {
	{ "max8906-codec", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max8906_i2c_id);

static struct i2c_driver max8906_i2c_driver = {
	.driver = {
		.name = "max8906-codec",
		.owner = THIS_MODULE,
	},
	.probe = max8906_i2c_probe,
	.remove = __devexit_p(max8906_i2c_remove),
	.id_table = max8906_i2c_id,
};

static int __init max8906_init(void)
{
	return i2c_add_driver(&max8906_i2c_driver);
}
module_init(max8906_init);

static void __exit max8906_exit(void)
{
	i2c_del_driver(&max8906_i2c_driver);
}
module_exit(max8906_exit);
MODULE_DESCRIPTION("ASoC MAX8906 amp driver");
MODULE_AUTHOR("KB_JetDroid");
MODULE_LICENSE("GPL");
