/*
 * Copyright 2012, 2014 Freescale Semiconductor, Inc.
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
#ifdef CONFIG_IWG15
/* IWG15: Audio: HeadPhone and Mic Detect Implementation */
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#endif

#include "../codecs/sgtl5000.h"
#include "imx-audmux.h"

#define DAI_NAME_SIZE	32

struct imx_sgtl5000_data {
	struct snd_soc_dai_link dai;
	struct snd_soc_card card;
	char codec_dai_name[DAI_NAME_SIZE];
	char platform_name[DAI_NAME_SIZE];
	struct clk *codec_clk;
	unsigned int clk_frequency;
};

#ifdef CONFIG_IWG15
/* IWG15: Audio: HeadPhone and Mic Detect Implementation */
struct imx_sgtl5000_priv {
        int hp_gpio;
        int hp_active_low;
        int mic_gpio;
        int mic_active_low;
        int hp_irq;
        int mic_irq;
};

static struct imx_sgtl5000_priv card_priv;
#endif

static int imx_sgtl5000_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	struct imx_sgtl5000_data *data = container_of(rtd->card,
					struct imx_sgtl5000_data, card);
	struct device *dev = rtd->card->dev;
	int ret;

	ret = snd_soc_dai_set_sysclk(rtd->codec_dai, SGTL5000_SYSCLK,
				     data->clk_frequency, SND_SOC_CLOCK_IN);
	if (ret) {
		dev_err(dev, "could not set codec driver clock params\n");
		return ret;
	}

	return 0;
}

static const struct snd_soc_dapm_widget imx_sgtl5000_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_LINE("Line In Jack", NULL),
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Line Out Jack", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
};

#ifdef CONFIG_IWG15
/* IWG15: Audio: HeadPhone and Mic Detect Implementation */
/* 
 *Headphone Detect Handler 
 */
static irqreturn_t hp_handler(int irq, void *dev_id)
{
	struct imx_sgtl5000_priv *priv = &card_priv;
	int hp_status;

	hp_status = gpio_get_value(priv->hp_gpio) ? 1 : 0;
	if (hp_status != priv->hp_active_low){
#ifdef CONFIG_IWG15
		printk("Headphone is plugged\n");
#else
		pr_debug("Headphone is plugged\n");
#endif
	}else {
#ifdef CONFIG_IWG15
		printk("Headphone is unplugged\n");
#else
		pr_debug("Headphone is unplugged\n");
#endif
	}
	return IRQ_RETVAL(1);
}

/*
 *Microphone Detect handler 
 */
static irqreturn_t mic_handler(int irq, void *dev_id)
{
	struct imx_sgtl5000_priv *priv = &card_priv;
	int mic_status;

	mic_status = gpio_get_value(priv->mic_gpio) ? 1 : 0;
	if (mic_status != priv->mic_active_low){
#ifdef CONFIG_IWG15
		printk("Microphone is plugged\n");
#else
		pr_debug("Microphone is plugged\n");
#endif
	}else {
#ifdef CONFIG_IWG15
		printk("Microphone is unplugged\n");
#else
		pr_debug("Microphone is unplugged\n");
#endif
	}
	return IRQ_RETVAL(1);
}
#endif

static int imx_sgtl5000_audmux_config(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int int_port, ext_port;
	int ret;

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
	ret = imx_audmux_v2_configure_port(int_port,
			IMX_AUDMUX_V2_PTCR_SYN |
			IMX_AUDMUX_V2_PTCR_TFSEL(ext_port) |
			IMX_AUDMUX_V2_PTCR_TCSEL(ext_port) |
			IMX_AUDMUX_V2_PTCR_TFSDIR |
			IMX_AUDMUX_V2_PTCR_TCLKDIR,
			IMX_AUDMUX_V2_PDCR_RXDSEL(ext_port));
	if (ret) {
		dev_err(&pdev->dev, "audmux internal port setup failed\n");
		return ret;
	}
	ret = imx_audmux_v2_configure_port(ext_port,
			IMX_AUDMUX_V2_PTCR_SYN,
			IMX_AUDMUX_V2_PDCR_RXDSEL(int_port));
	if (ret) {
		dev_err(&pdev->dev, "audmux external port setup failed\n");
		return ret;
	}

	return 0;
}

static int imx_sgtl5000_probe(struct platform_device *pdev)
{
	struct device_node *cpu_np, *codec_np;
	struct platform_device *cpu_pdev;
	struct i2c_client *codec_dev;
	struct imx_sgtl5000_data *data;
#ifdef CONFIG_IWG15M_MXM
	/* IWG15: Audio: HeadPhone and Mic Detect Implementation */
	struct imx_sgtl5000_priv *priv = &card_priv;
	int hp_status,mic_status;
	struct device_node *np = pdev->dev.of_node;
#endif
	int ret;

	cpu_np = of_parse_phandle(pdev->dev.of_node, "cpu-dai", 0);
	codec_np = of_parse_phandle(pdev->dev.of_node, "audio-codec", 0);
	if (!cpu_np || !codec_np) {
		dev_err(&pdev->dev, "phandle missing or invalid\n");
		ret = -EINVAL;
		goto fail;
	}

	if (strstr(cpu_np->name, "ssi")) {
		ret = imx_sgtl5000_audmux_config(pdev);
		if (ret)
			goto fail;
	}

	cpu_pdev = of_find_device_by_node(cpu_np);
	if (!cpu_pdev) {
		dev_err(&pdev->dev, "failed to find SSI platform device\n");
		ret = -EINVAL;
		goto fail;
	}
	codec_dev = of_find_i2c_device_by_node(codec_np);
	if (!codec_dev) {
		dev_err(&pdev->dev, "failed to find codec platform device\n");
		return -EINVAL;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto fail;
	}

	data->codec_clk = clk_get(&codec_dev->dev, NULL);
	if (IS_ERR(data->codec_clk)) {
		/* assuming clock enabled by default */
		data->codec_clk = NULL;
		ret = of_property_read_u32(codec_np, "clock-frequency",
					&data->clk_frequency);
		if (ret) {
			dev_err(&codec_dev->dev,
				"clock-frequency missing or invalid\n");
			goto fail;
		}
	} else {
		data->clk_frequency = clk_get_rate(data->codec_clk);
		clk_prepare_enable(data->codec_clk);
	}

#ifdef CONFIG_IWG15M_MXM
	/* IWG15: Audio: HeadPhone and Mic Detect Implementation */

	priv->hp_gpio = of_get_named_gpio_flags(np, "hp-det-gpios", 0,
			(enum of_gpio_flags *)&priv->hp_active_low);
	priv->mic_gpio = of_get_named_gpio_flags(np, "mic-det-gpios", 0,
			(enum of_gpio_flags *)&priv->mic_active_low);

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
#endif

	data->dai.name = "HiFi";
	data->dai.stream_name = "HiFi";
	data->dai.codec_dai_name = "sgtl5000";
	data->dai.codec_of_node = codec_np;
	data->dai.cpu_of_node = cpu_np;
	data->dai.platform_of_node = cpu_np;
	data->dai.init = &imx_sgtl5000_dai_init;
	data->dai.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
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
	data->card.dapm_widgets = imx_sgtl5000_dapm_widgets;
	data->card.num_dapm_widgets = ARRAY_SIZE(imx_sgtl5000_dapm_widgets);

	ret = snd_soc_register_card(&data->card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", ret);
		goto clk_fail;
	}

	platform_set_drvdata(pdev, data);
clk_fail:
	clk_put(data->codec_clk);
fail:
	if (cpu_np)
		of_node_put(cpu_np);
	if (codec_np)
		of_node_put(codec_np);

	return ret;
}

static int imx_sgtl5000_remove(struct platform_device *pdev)
{
	struct imx_sgtl5000_data *data = platform_get_drvdata(pdev);

	if (data->codec_clk) {
		clk_disable_unprepare(data->codec_clk);
		clk_put(data->codec_clk);
	}
	snd_soc_unregister_card(&data->card);

	return 0;
}

static const struct of_device_id imx_sgtl5000_dt_ids[] = {
	{ .compatible = "fsl,imx-audio-sgtl5000", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_sgtl5000_dt_ids);

static struct platform_driver imx_sgtl5000_driver = {
	.driver = {
		.name = "imx-sgtl5000",
		.owner = THIS_MODULE,
		.of_match_table = imx_sgtl5000_dt_ids,
	},
	.probe = imx_sgtl5000_probe,
	.remove = imx_sgtl5000_remove,
};
module_platform_driver(imx_sgtl5000_driver);

MODULE_AUTHOR("Shawn Guo <shawn.guo@linaro.org>");
MODULE_DESCRIPTION("Freescale i.MX SGTL5000 ASoC machine driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:imx-sgtl5000");
