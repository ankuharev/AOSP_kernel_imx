/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 * Copyright 2012 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_i2c.h>
#include <linux/clk.h>
#include <sound/soc.h>

#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/fsl_devices.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#include "imx-ssi.h"

#include "imx-pcm.h"

#include "../codecs/rt5610.h"
#include "imx-audmux.h"

#define DAI_NAME_SIZE	32

struct imx_rt5610_data {
        struct snd_soc_dai_link dai;
        struct snd_soc_card card;
        char codec_dai_name[DAI_NAME_SIZE];
        char platform_name[DAI_NAME_SIZE];
        struct clk *codec_clk;
        unsigned int clk_frequency;
};
struct imx_rt5610_priv {
        int hp_gpio;
        int hp_active_low;
	int mic_gpio;
        int mic_active_low;
        int hp_irq;
        int mic_irq;
        int sysclk;
        int hw;
	struct snd_soc_codec *codec;
        struct platform_device *pdev;
};

static struct imx_rt5610_priv card_priv;

static int rt5610_jack_func;
static int rt5610_spk_func;
static int rt5610_line_in_func;
static int rt5610_mic_jack_func;

static const char *jack_function[] = { "off", "on"};

static const char *spk_function[] = { "off", "on" };

static const char *line_in_function[] = { "off", "on" };

static const char *mic_jack_function[] = { "off", "on"};

static const struct soc_enum rt5610_enum[] = {
        SOC_ENUM_SINGLE_EXT(2, jack_function),
        SOC_ENUM_SINGLE_EXT(2, spk_function),
        SOC_ENUM_SINGLE_EXT(2, line_in_function),
        SOC_ENUM_SINGLE_EXT(2, mic_jack_function),
};

static int rt5610_get_jack(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
        ucontrol->value.enumerated.item[0] = rt5610_jack_func;
        return 0;
}

static int rt5610_set_jack(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

        if (rt5610_jack_func == ucontrol->value.enumerated.item[0])
                return 0;

        rt5610_jack_func = ucontrol->value.enumerated.item[0];
        if (rt5610_jack_func)
                snd_soc_dapm_enable_pin(codec, "Headphone Jack");
        else
                snd_soc_dapm_disable_pin(codec, "Headphone Jack");

        snd_soc_dapm_sync(codec);
        return 1;
}

static int rt5610_get_spk(struct snd_kcontrol *kcontrol,
                            struct snd_ctl_elem_value *ucontrol)
{
        ucontrol->value.enumerated.item[0] = rt5610_spk_func;
        return 0;
}

static int rt5610_set_spk(struct snd_kcontrol *kcontrol,
                            struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

        if (rt5610_spk_func == ucontrol->value.enumerated.item[0])
                return 0;

        rt5610_spk_func = ucontrol->value.enumerated.item[0];
        if (rt5610_spk_func)
                snd_soc_dapm_enable_pin(codec, "Ext Spk");
        else
                snd_soc_dapm_disable_pin(codec, "Ext Spk");

        snd_soc_dapm_sync(codec);
        return 1;
}

static int rt5610_get_line_in(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
        ucontrol->value.enumerated.item[0] = rt5610_line_in_func;
        return 0;
}

static int rt5610_set_line_in(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

        if (rt5610_line_in_func == ucontrol->value.enumerated.item[0])
                return 0;

        rt5610_line_in_func = ucontrol->value.enumerated.item[0];
        if (rt5610_line_in_func)
                snd_soc_dapm_enable_pin(codec, "Line In Jack");
        else
                snd_soc_dapm_disable_pin(codec, "Line In Jack");

        snd_soc_dapm_sync(codec);
        return 1;
}

static int rt5610_get_mic_jack(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
        ucontrol->value.enumerated.item[0] = rt5610_mic_jack_func;
        return 0;
}

static int rt5610_set_mic_jack(struct snd_kcontrol *kcontrol,
                             struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

        if (rt5610_mic_jack_func == ucontrol->value.enumerated.item[0])
                return 0;

        rt5610_mic_jack_func = ucontrol->value.enumerated.item[0];
        if (rt5610_mic_jack_func)
                snd_soc_dapm_enable_pin(codec, "Mic Jack");
        else
                snd_soc_dapm_disable_pin(codec, "Mic Jack");

        snd_soc_dapm_sync(codec);
        return 1;
}

static int imx_rt5610_init(struct snd_soc_pcm_runtime *rtd)
{
        struct snd_soc_codec *codec = rtd->codec;
        struct imx_rt5610_priv *priv = &card_priv;

        priv->codec = codec;

        snd_soc_dapm_disable_pin(&codec->dapm, "Line In Jack");

        snd_soc_dapm_sync(&codec->dapm);

        return 0;
}

static const struct snd_soc_dapm_route audio_map[] = {
        /* Mic Jack --> MIC1 (with automatic bias) */
        {"MIC Bias1", NULL, "Mic Jack"},
        {"MIC1", NULL, "MIC Bias1"},
        /* Line in Jack --> MIC2*/
        {"MIC Bias2", NULL, "Line In Jack"},
        {"MIC2", NULL, "MIC Bias2"},
        /* HPO --> Headphone Jack */
        {"Headphone Jack", NULL, "HPOL"},
        {"Headphone Jack", NULL, "HPOR"},
        /* SPO --> Ext Speaker */
        {"Ext Spk", NULL, "SPOL"},
        {"Ext Spk", NULL, "SPOR"},
};

static const struct snd_kcontrol_new rt5610_machine_controls[] = {
        SOC_ENUM_EXT("Jack Function", rt5610_enum[0], rt5610_get_jack,
                     rt5610_set_jack),
        SOC_ENUM_EXT("Speaker Function", rt5610_enum[1], rt5610_get_spk,
                     rt5610_set_spk),
        SOC_ENUM_EXT("Line In Function", rt5610_enum[2], rt5610_get_line_in,
                     rt5610_set_line_in),
        SOC_ENUM_EXT("Mic Jack Function", rt5610_enum[3], rt5610_get_mic_jack,
                     rt5610_set_mic_jack),
};

static const struct snd_soc_dapm_widget imx_rt5610_dapm_widgets[] = {
        SND_SOC_DAPM_MIC("Mic Jack", NULL),
        SND_SOC_DAPM_LINE("Line In Jack", NULL),
        SND_SOC_DAPM_SPK("Ext Spk", NULL),
        SND_SOC_DAPM_HP("Headphone Jack", NULL),
};

static void headphone_detect_handler(struct work_struct *work)
{
        struct imx_rt5610_priv *priv = &card_priv;
        struct platform_device *pdev = priv->pdev;
        int hp_status;
        char *envp[3];
        char *buf;

        sysfs_notify(&pdev->dev.kobj, NULL, "headphone");
        /* setup a message for userspace headphone in */
        buf = kmalloc(32, GFP_ATOMIC);
        if (!buf) {
                pr_err("%s kmalloc failed\n", __func__);
                return;
        }
        envp[0] = "NAME=headphone";
        snprintf(buf, 32, "STATE=%d", hp_status);
        envp[1] = buf;
        envp[2] = NULL;
        kobject_uevent_env(&pdev->dev.kobj, KOBJ_CHANGE, envp);
        kfree(buf);
        printk(KERN_INFO "Headphone status is = %d\n",hp_status);

	hp_status = gpio_get_value(priv->hp_gpio) ? 1 : 0;
	if (hp_status != priv->hp_active_low){
		irq_set_irq_type(priv->hp_gpio, IRQF_TRIGGER_FALLING);
		printk(KERN_INFO "Headphone is detected \n");
	}else {
		irq_set_irq_type(priv->hp_gpio, IRQF_TRIGGER_RISING);
		printk(KERN_INFO "Speaker is detected \n");
	}
        enable_irq(priv->hp_gpio);
}

static DECLARE_DELAYED_WORK(hp_event, headphone_detect_handler);

static void mic_detect_handler(struct work_struct *work)
{
	struct imx_rt5610_priv *priv = &card_priv;
	struct platform_device *pdev = priv->pdev;
	int mic_status;
	char *envp[3];
	char *buf;

	sysfs_notify(&pdev->dev.kobj, NULL, "microphone");
	mic_status = gpio_get_value(priv->mic_gpio) ? 1 : 0;
	/* setup a message for userspace headphone in */
	buf = kmalloc(32, GFP_ATOMIC);
	if (!buf) {
		pr_err("%s kmalloc failed\n", __func__);
		return;
	}
	envp[0] = "NAME=microphone";
	snprintf(buf, 32, "STATE=%d", mic_status);
	envp[1] = buf;
	envp[2] = NULL;
	kobject_uevent_env(&pdev->dev.kobj, KOBJ_CHANGE, envp);
	kfree(buf);
	printk(KERN_INFO "Microphone status is = %d\n",mic_status);

	if (mic_status != priv->mic_active_low){
		printk(KERN_INFO "Microphone is detected \n");
		irq_set_irq_type(priv->mic_gpio, IRQF_TRIGGER_FALLING);
	} else {
		printk(KERN_INFO "Line in jack is detected \n");
		irq_set_irq_type(priv->mic_gpio, IRQF_TRIGGER_RISING);
		enable_irq(priv->mic_gpio);
	}
}

static DECLARE_DELAYED_WORK(mp_event, mic_detect_handler);

static ssize_t show_headphone(struct device_driver *dev, char *buf)
{
        struct imx_rt5610_priv *priv = &card_priv;
        u16 hp_status;

	/* determine whether hp is plugged in */
	hp_status = gpio_get_value(priv->hp_gpio) ? 1 : 0;
	if (hp_status != priv->hp_active_low)
		strcpy(buf, "headphone\n");
	else
		strcpy(buf, "speaker\n");

	return strlen(buf);
}

static DRIVER_ATTR(headphone, S_IRUGO | S_IWUSR, show_headphone, NULL);

static ssize_t show_mic(struct device_driver *dev, char *buf)
{
	struct imx_rt5610_priv *priv = &card_priv;
	u16 mic_status;

	/* determine whether hp is plugged in */
	mic_status = gpio_get_value(priv->mic_gpio) ? 1 : 0;
	if (mic_status != priv->mic_active_low)
		strcpy(buf, "microphone\n");
	else
		strcpy(buf, "Line in jack\n");


	return strlen(buf);
}

static DRIVER_ATTR(microphone, S_IRUGO | S_IWUSR, show_mic, NULL);

static int imx_3stack_audio_hw_params(struct snd_pcm_substream *substream,
                                      struct snd_pcm_hw_params *params)
{
        struct snd_soc_pcm_runtime *rtd = substream->private_data;
        struct snd_soc_dai_link *machine = rtd->dai_link;
        struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
        struct snd_soc_dai *codec_dai = rtd->codec_dai;
        struct imx_rt5610_priv *priv = &card_priv;
        unsigned int rate = params_rate(params);

        unsigned int channels = params_channels(params);

        /* only need to do this once as capture and playback are sync */
        if (priv->hw)
                return 0;
        priv->hw = 1;

//        snd_soc_dai_set_sysclk(codec_dai, 0, 22579200, SND_SOC_CLOCK_IN);

        /* set the SSI system clock as input (unused) */
//        snd_soc_dai_set_sysclk(cpu_dai, IMX_SSP_SYS_CLK, 0, SND_SOC_CLOCK_IN);
        return 0;
}

static int imx_3stack_startup(struct snd_pcm_substream *substream)
{
        return 0;
}

static void imx_3stack_shutdown(struct snd_pcm_substream *substream)
{
        struct imx_rt5610_priv *priv = &card_priv;

        priv->hw = 0;
}

/* 
 *Headphone Detect Handler 
 */
static irqreturn_t hp_handler(int irq, void *dev_id)
{
	struct imx_rt5610_priv *priv = &card_priv;
	struct platform_device *pdev = priv->pdev;
	int hp_status;

	hp_status = gpio_get_value(priv->hp_gpio) ? 1 : 0;
	if (hp_status != priv->hp_active_low){
		printk("Headphone is plugged\n");
	}else {
		printk("Headphone is unplugged\n");
	}
	return IRQ_RETVAL(1);
}

/*
 *Microphone Detect handler 
 */
static irqreturn_t mic_handler(int irq, void *dev_id)
{
        struct imx_rt5610_priv *priv = &card_priv;
        struct platform_device *pdev = priv->pdev;
        int mic_status;

        mic_status = gpio_get_value(priv->mic_gpio) ? 1 : 0;
        if (mic_status != priv->mic_active_low){
                printk("Microphone is plugged\n");
        }else {
                printk("Microphone is unplugged\n");
        }
        return IRQ_RETVAL(1);
}

/*
 * imx_3stack RT5610 audio DAI opserations.
 */
static struct snd_soc_ops imx_3stack_ops = {
        .startup = imx_3stack_startup,
        .shutdown = imx_3stack_shutdown,
        .hw_params = imx_3stack_audio_hw_params,
};

static int imx_rt5610_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *ssi_np, *codec_np;
	struct platform_device *ssi_pdev;
        struct imx_rt5610_priv *priv = &card_priv;
	struct imx_rt5610_data *data;
	int int_port, ext_port;
	int ret;

        unsigned int ssi_ptcr = 0;
        unsigned int dai_ptcr = 0;
        unsigned int ssi_pdcr = 0;
        unsigned int dai_pdcr = 0;

	ret = of_property_read_u32(np, "mux-int-port", &int_port);
	if (ret) {
		dev_err(&pdev->dev, "mux-int-port missing or invalid\n");
		return ret;
	}
	ret = of_property_read_u32(np, "mux-ext-port", &ext_port);
	if (ret) {
		dev_err(&pdev->dev, "mux-ext-port missing or invalid\n");
		return ret;
	}

	/*
	 * The port numbering in the hardware manual starts at 1, while
	 * the audmux API expects it starts at 0.
	 */
	int_port--;
	ext_port--;

        ssi_ptcr |= IMX_AUDMUX_V2_PTCR_SYN;
        ssi_ptcr &= ~IMX_AUDMUX_V2_PTCR_TFSDIR;
        ssi_ptcr |= IMX_AUDMUX_V2_PTCR_TFSEL(ext_port);
        ssi_ptcr |= IMX_AUDMUX_V2_PTCR_TCLKDIR;
        ssi_ptcr |= IMX_AUDMUX_V2_PTCR_TCSEL(ext_port);

        ssi_pdcr |= IMX_AUDMUX_V2_PDCR_RXDSEL(ext_port);

        ret = imx_audmux_v2_configure_port(int_port, ssi_ptcr, ssi_pdcr);
        if (ret) {
                dev_err(&pdev->dev, "audmux internal port setup failed\n");
                return ret;
        }
        dai_ptcr |= IMX_AUDMUX_V2_PTCR_SYN;
        dai_ptcr |= IMX_AUDMUX_V2_PTCR_TFSDIR;
        dai_ptcr |= IMX_AUDMUX_V2_PTCR_TFSEL(int_port);
        dai_ptcr &= ~IMX_AUDMUX_V2_PTCR_TCLKDIR;
        dai_ptcr |= IMX_AUDMUX_V2_PTCR_TCSEL(int_port);

        dai_pdcr |= IMX_AUDMUX_V2_PDCR_RXDSEL(int_port);

        /* Enabled TXRXEN Bit as per H/W. Only for PRDVZ */
        dai_pdcr |= IMX_AUDMUX_V2_PDCR_TXRXEN;

        ret = imx_audmux_v2_configure_port(ext_port,dai_ptcr, dai_pdcr);
        if (ret) {
                dev_err(&pdev->dev, "audmux external port setup failed\n");
                return ret;
        }
	ssi_np = of_parse_phandle(pdev->dev.of_node, "ssi-controller", 0);
	codec_np = of_parse_phandle(pdev->dev.of_node, "audio-codec", 0);
	if (!ssi_np || !codec_np) {
		dev_err(&pdev->dev, "phandle missing or invalid\n");
		ret = -EINVAL;
		goto fail;
	}

	ssi_pdev = of_find_device_by_node(ssi_np);
	if (!ssi_pdev) {
		dev_err(&pdev->dev, "failed to find SSI platform device\n");
		ret = -EINVAL;
		goto fail;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto fail;
	}

	priv->hp_gpio = of_get_named_gpio_flags(np, "hp-det-gpios", 0,
                                (enum of_gpio_flags *)&priv->hp_active_low);
	priv->mic_gpio = of_get_named_gpio_flags(np, "mic-det-gpios", 0,
                                (enum of_gpio_flags *)&priv->mic_active_low);

	int hp_status,mic_status;
	gpio_request(priv->hp_gpio, "HeadPhone-Detect");
	gpio_direction_input(priv->hp_gpio);
	priv->hp_irq = gpio_to_irq(priv->hp_gpio);
	hp_status = gpio_get_value(priv->hp_gpio) ? 1 : 0;
	if (hp_status != priv->hp_active_low){
		irq_set_irq_type(priv->hp_irq, IRQF_TRIGGER_FALLING);
	}else {
		irq_set_irq_type(priv->hp_irq, IRQF_TRIGGER_RISING);

	}
	ret=request_irq(priv->hp_irq,hp_handler,IRQF_DISABLED | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"HeadPhone",0);
	if (ret < 0) {
		printk(KERN_ALERT "%s: request hp_irq failed with %d\n",__func__, ret);
		free_irq(priv->hp_irq,0);
	}
	
	gpio_request(priv->mic_gpio, "Microphone-Detect");
	gpio_direction_input(priv->mic_gpio);
	priv->mic_irq = gpio_to_irq(priv->mic_gpio);
	mic_status = gpio_get_value(priv->mic_gpio) ? 1 : 0;
	if (mic_status != priv->mic_active_low){
		irq_set_irq_type(priv->mic_irq, IRQF_TRIGGER_FALLING);
	}else {
		irq_set_irq_type(priv->mic_irq, IRQF_TRIGGER_RISING);

	}
	ret=request_irq(priv->mic_irq,mic_handler,IRQF_DISABLED | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"MicroPhone",0);
	if (ret < 0) {
		printk(KERN_ALERT "%s: request mic_irq failed with %d\n",__func__, ret);
		free_irq(priv->mic_irq,0);
	}

	data->dai.name = "RT5610";
	data->dai.stream_name = "rt5610 HIFI";
	data->dai.codec_dai_name = "rt5610-hifi";
	data->dai.codec_of_node = codec_np;
	data->dai.cpu_of_node = ssi_np;
	data->dai.platform_of_node = ssi_np;
	data->dai.ops = &imx_3stack_ops;
	data->dai.init = &imx_rt5610_init;
	data->dai.dai_fmt = SND_SOC_DAIFMT_AC97 | SND_SOC_DAIFMT_NB_NF |
			    SND_SOC_DAIFMT_CBM_CFM;

	data->card.dev = &pdev->dev;
	ret = snd_soc_of_parse_card_name(&data->card, "model");
	if (ret)
		goto clk_fail;
	ret = snd_soc_of_parse_audio_routing(&data->card, "audio-routing");
	if (ret)
		goto clk_fail;
	data->card.num_links = 1;
	data->card.owner = THIS_MODULE;
	data->card.dai_link = &data->dai;
	data->card.dapm_widgets = imx_rt5610_dapm_widgets;
	data->card.num_dapm_widgets = ARRAY_SIZE(imx_rt5610_dapm_widgets);
	data->card.controls = rt5610_machine_controls;
	data->card.num_controls = ARRAY_SIZE(rt5610_machine_controls);
	data->card.dapm_routes = audio_map;
	data->card.num_dapm_routes = ARRAY_SIZE(audio_map);

	ret = snd_soc_register_card(&data->card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);
		goto clk_fail;
	}

	platform_set_drvdata(pdev, data);
clk_fail:
	clk_put(data->codec_clk);
fail:
	if (ssi_np)
		of_node_put(ssi_np);
	if (codec_np)
		of_node_put(codec_np);

	return ret;
}

static int imx_rt5610_remove(struct platform_device *pdev)
{
	struct imx_rt5610_data *data = platform_get_drvdata(pdev);
	struct imx_rt5610_priv *priv = &card_priv;

	free_irq(priv->hp_irq,0);
	free_irq(priv->mic_irq,0);
	snd_soc_unregister_card(&data->card);

	return 0;
}

static const struct of_device_id imx_rt5610_dt_ids[] = {
	{ .compatible = "fsl,imx-audio-rt5610", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_rt5610_dt_ids);

static struct platform_driver imx_rt5610_driver = {
	.driver = {
		.name = "imx-rt5610",
		.owner = THIS_MODULE,
		.of_match_table = imx_rt5610_dt_ids,
	},
	.probe = imx_rt5610_probe,
	.remove = imx_rt5610_remove,
};
module_platform_driver(imx_rt5610_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("RT5610 Driver for i.MX 3STACK");
MODULE_LICENSE("GPL");
