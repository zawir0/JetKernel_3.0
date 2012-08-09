/*
 * gt_s8000_max9880_max8906.c  --  SoC audio for Samsung GT-S8000 (Jet)
 *
 * Copyright (C) 2012 KB_JetDroid <kbjetdroid@gmail.com>
 *
 * Created based on gt_i5700_ak4671_max9877.c by Tomasz Figa <tomasz.figa at gmail.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/i2c/max8906.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/soc-dai.h>
#include <sound/gt_s8000.h>

#include <asm/mach-types.h>

#include "dma.h"
#include "i2s.h"

#include "../codecs/ak4671.h"
#include "../codecs/max9877.h"

static struct snd_soc_card gt_s8000;
static struct gt_s8000_audio_pdata *gt_s8000_pdata;

#define GPIO_FM_LDO_ON		S3C64XX_GPN(4)
#define GPIO_FM_RST			S3C64XX_GPK(5)

static int gt_s8000_hifi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
					     SND_SOC_DAIFMT_NB_NF |
					     SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
					   SND_SOC_DAIFMT_NB_NF |
					   SND_SOC_DAIFMT_CBM_CFM);//SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Use PCLK for I2S signal generation */
	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_RCLKSRC_1,
							0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	/* Gate the RCLK output on PAD */
	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK,
							0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	/* set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 12000000, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * GT-S8000 MAX9880 HiFi DAI operations.
 */
static struct snd_soc_ops gt_s8000_hifi_ops = {
	.hw_params = gt_s8000_hifi_hw_params,
};

DEFINE_MUTEX(mic_lock);
static int mic_enabled = 0;

#define MIC_MAIN	(1 << 0)
#define MIC_SUB		(1 << 1)
#define MIC_JACK	(1 << 2)

static int gt_s8000_main_mic_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	int en = 0;
	mutex_lock(&mic_lock);

	if (SND_SOC_DAPM_EVENT_ON(event))
	{
		mic_enabled |= MIC_MAIN;
		en = 1;
	}
	else
		mic_enabled &= ~MIC_MAIN;

	//gt_s8000_pdata->set_micbias(!!mic_enabled);
	if (Set_MAX8906_PM_REG(WMEMEN, en))
		printk("Successfully enabled regulator WMEMEN\n");

	mutex_unlock(&mic_lock);

	return 0;
}

static int gt_s8000_fm_receive_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	int en = 0;
	return 0;
	mutex_lock(&mic_lock);

	if (SND_SOC_DAPM_EVENT_ON(event))
	{
		gpio_set_value(GPIO_FM_LDO_ON, 1);
		mdelay(100);
		gpio_set_value(GPIO_FM_RST, 0);
		mdelay(100);
		gpio_set_value(GPIO_FM_RST, 1);
	}
	else
	{
		gpio_set_value(GPIO_FM_LDO_ON, 0);
	}
	//gt_s8000_pdata->set_micbias(!!mic_enabled);
//	if (Set_MAX8906_PM_REG(WMEMEN, en))
		printk("KB: %s fm receive\n", __func__);

	mutex_unlock(&mic_lock);

	return 0;
}

static int gt_s8000_sub_mic_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	int en = 0;
	mutex_lock(&mic_lock);

	if (SND_SOC_DAPM_EVENT_ON(event))
	{
		mic_enabled |= MIC_SUB;
		en = 1;
	}
	else
		mic_enabled &= ~MIC_SUB;

//	gt_s8000_pdata->set_micbias(!!mic_enabled);
	if (Set_MAX8906_PM_REG(WMEMEN, en))
		printk("Successfully enabled regulator WMEMEN\n");

	mutex_unlock(&mic_lock);

	return 0;
}

static int gt_s8000_jack_mic_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	mutex_lock(&mic_lock);

	if (SND_SOC_DAPM_EVENT_ON(event))
		mic_enabled |= MIC_JACK;
	else
		mic_enabled &= ~MIC_JACK;

//	gt_s8000_pdata->set_micbias(!!mic_enabled);

	mutex_unlock(&mic_lock);

	return 0;
}

static const struct snd_kcontrol_new gt_s8000_direct_controls[] = {
	SOC_DAPM_PIN_SWITCH("Earpiece"),
//	SOC_DAPM_PIN_SWITCH("GSM Send"),
	SOC_DAPM_PIN_SWITCH("Main Mic"),
	SOC_DAPM_PIN_SWITCH("Ear Mic"),
	SOC_DAPM_PIN_SWITCH("FM Receive"),
	SOC_DAPM_PIN_SWITCH("GSM Receive"),
};

static const struct snd_kcontrol_new gt_s8000_amp_controls[] = {
	SOC_DAPM_PIN_SWITCH("Headphones"),
	SOC_DAPM_PIN_SWITCH("Speaker"),
};

static const struct snd_soc_dapm_widget gt_s8000_dapm_direct_widgets[] = {
	SND_SOC_DAPM_LINE("Earpiece", NULL),
//	SND_SOC_DAPM_LINE("GSM Send", NULL),
	SND_SOC_DAPM_MIC("Main Mic", gt_s8000_main_mic_event),
	SND_SOC_DAPM_MIC("Ear Mic", gt_s8000_sub_mic_event),
	SND_SOC_DAPM_MIC("GSM Receive", NULL),
	SND_SOC_DAPM_LINE("FM Receive", gt_s8000_fm_receive_event),
};

static const struct snd_soc_dapm_widget gt_s8000_dapm_amp_widgets[] = {
	SND_SOC_DAPM_HP("Headphones", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
};

static const struct snd_soc_dapm_route dapm_direct_routes[] = {
	/* Codec outputs */
	{"Earpiece", NULL, "LOUTP"},
	//{"Earpiece", NULL, "ROUT1"},
	//{"GSM Send", NULL, "LOUT3"},
	//{"GSM Send", NULL, "ROUT3"},

	/* Codec inputs */
	{"MMICIN", NULL, "Main Mic"},
	//{"RIN1", NULL, "Main Mic"},
	{"EMICIN", NULL, "Ear Mic"},
	//{"RIN2", NULL, "Sub Mic"},
	{"LLINEIN", NULL, "FM Receive"},
	{"RLINEIN", NULL, "FM Receive"},
};

static const struct snd_soc_dapm_route dapm_amp_routes[] = {
	/* Amplifier inputs */
	{"VOICEINP", NULL, "GSM Receive"},
	{"VOICEINN", NULL, "GSM Receive"},
	{"IN3", NULL, "LOUTL"},
	{"IN4", NULL, "LOUTR"},

	/* Amplifier outputs */
	{"Headphones", NULL, "HPL"},
	{"Headphones", NULL, "HPR"},
	{"Speaker", NULL, "OUT"},
};

/*
 * This is an example machine initialisation for a max9880 connected to a
 * gt_s8000. It is missing logic to detect hp/mic insertions and logic
 * to re-route the audio in such an event.
 */
static int gt_s8000_max9880_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int err;

	pr_debug("Entered %s\n", __func__);

	/* Add gt_s8000 specific widgets */
	err = snd_soc_dapm_new_controls(dapm, gt_s8000_dapm_direct_widgets,
				ARRAY_SIZE(gt_s8000_dapm_direct_widgets));
	if (err < 0)
		return err;

	err = snd_soc_add_controls(codec, gt_s8000_direct_controls,
					ARRAY_SIZE(gt_s8000_direct_controls));
	if (err < 0)
		return err;

	/* set up gt_s8000 specific audio routes */
	err = snd_soc_dapm_add_routes(dapm, dapm_direct_routes,
						ARRAY_SIZE(dapm_direct_routes));
	if (err < 0)
		return err;

	snd_soc_dapm_disable_pin(dapm, "Earpiece");
	//snd_soc_dapm_disable_pin(dapm, "GSM Send");
	snd_soc_dapm_disable_pin(dapm, "Main Mic");
	snd_soc_dapm_disable_pin(dapm, "Ear Mic");
	snd_soc_dapm_disable_pin(dapm, "FM Receive");
	snd_soc_dapm_disable_pin(dapm, "GSM Receive");
	snd_soc_dapm_ignore_suspend(dapm, "Earpiece");
	//snd_soc_dapm_ignore_suspend(dapm, "GSM Send");
	snd_soc_dapm_ignore_suspend(dapm, "Main Mic");
	snd_soc_dapm_ignore_suspend(dapm, "Ear Mic");
	snd_soc_dapm_ignore_suspend(dapm, "FM Receive");
	snd_soc_dapm_ignore_suspend(dapm, "GSM Receive");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static int gt_s8000_max8906_init(struct snd_soc_dapm_context *dapm)
{
	int err;
	/* Add gt_s8000 specific widgets */
	err = snd_soc_dapm_new_controls(dapm, gt_s8000_dapm_amp_widgets,
					ARRAY_SIZE(gt_s8000_dapm_amp_widgets));
	if (err < 0)
		return err;

	err = snd_soc_add_controls(dapm->codec, gt_s8000_amp_controls,
					ARRAY_SIZE(gt_s8000_amp_controls));
	if (err < 0)
		return err;

	/* set up gt_s8000 specific audio routes */
	err = snd_soc_dapm_add_routes(dapm, dapm_amp_routes,
						ARRAY_SIZE(dapm_amp_routes));
	if (err < 0)
		return err;

	snd_soc_dapm_nc_pin(dapm, "SPKBPIN");
	snd_soc_dapm_disable_pin(dapm, "Headphones");
	snd_soc_dapm_disable_pin(dapm, "Speaker");
	snd_soc_dapm_ignore_suspend(dapm, "Headphones");
	snd_soc_dapm_ignore_suspend(dapm, "Speaker");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link gt_s8000_dai[] = {
	{ /* Hifi Playback - for similatious use with voice below */
		.name = "max9880",
		.stream_name = "max9880 HiFi",
		.platform_name = "samsung-audio",
		.cpu_dai_name = "samsung-i2s.0",
		.codec_dai_name = "max9880-hifi",
		.codec_name = "max9880-codec.5-0010",
		.init = gt_s8000_max9880_init,
		.ops = &gt_s8000_hifi_ops,
	},
};

static struct snd_soc_aux_dev gt_s8000_aux[] = {
	{ /* Headphone/Speaker amplifier */
		.name = "max8906-codec",
		.codec_name = "max8906-codec.5-003c",
		.init = gt_s8000_max8906_init,
	},
};

static struct snd_soc_card gt_s8000 = {
	.name		= "GT-S8000",
	.dai_link	= gt_s8000_dai,
	.num_links	= ARRAY_SIZE(gt_s8000_dai),
	.aux_dev	= gt_s8000_aux,
	.num_aux_devs	= ARRAY_SIZE(gt_s8000_aux),
};

static int __init gt_s8000_probe(struct platform_device *pdev)
{
	struct gt_s8000_audio_pdata *pdata = pdev->dev.platform_data;
	struct platform_device *audio_pdev;
	int ret = 0;

	if (!machine_is_gt_S8000()) {
		dev_err(&pdev->dev, "Only GT-S8000 is supported by this ASoC driver\n");
		return -ENODEV;
	}

	if (pdev->id != -1) {
		dev_err(&pdev->dev, "Only a single instance is allowed\n");
		return -ENODEV;
	}

	if (!pdata) {
		dev_err(&pdev->dev, "No platform data specified\n");
		return -EINVAL;
	}

#if 0
	if (!pdata->set_micbias) {
		dev_err(&pdev->dev, "No micbias control function provided\n");
		return -EINVAL;
	}
#endif

	audio_pdev = platform_device_alloc("soc-audio", -1);
	if (!audio_pdev)
		return -ENOMEM;

	platform_set_drvdata(audio_pdev, &gt_s8000);

	gt_s8000_pdata = pdata;

	ret = platform_device_add(audio_pdev);
	if (ret)
		goto err_free_pdev;

	return 0;

err_free_pdev:
	platform_device_put(audio_pdev);

	return ret;
}

static void gt_s8000_shutdown(struct platform_device *pdev)
{
	struct gt_s8000_audio_pdata *pdata = pdev->dev.platform_data;
	//TODO: Disable MAX8906 Audio
	//pdata->set_micbias(0);
}

static struct platform_driver gt_s8000_driver = {
	.shutdown = gt_s8000_shutdown,
	.driver = {
		.name = "gt_s8000_audio",
	},
};

static int __init gt_s8000_init(void)
{
	return platform_driver_probe(&gt_s8000_driver, gt_s8000_probe);
}

module_init(gt_s8000_init);

/* Module information */
MODULE_AUTHOR("KB_JetDroid");
MODULE_DESCRIPTION("ALSA SoC MAX9880 MAX8906 GT-S8000");
MODULE_LICENSE("GPL");
