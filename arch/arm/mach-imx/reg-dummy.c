/*
 * Copyright (c) 2014 iWave Systems Technologies Pvt. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
/*
 * @file reg-dummy.c
 *
 * @brief Simple driver for dummy regulator
 *
 * @ingroup IWG15M
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

static struct regulator_dev *arm_dummy_regulator_rdev;
static struct regulator_dev *soc_dummy_regulator_rdev;
static struct regulator_init_data dummy_initdata = {
	.constraints = {
		.max_uV = 1450000,	/* allign with real max of anatop */
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_VOLTAGE,
	},
};
static int arm_dummy_enable,soc_dummy_enable;

static int reg_set_voltage(struct regulator_dev *reg, int min_uV,
					int max_uV, unsigned *selector)
{
	return 0;
}

static int reg_list_voltage(struct regulator_dev *rdev,
				unsigned int selector)
{
	return 0;
}

static int armreg_enable(struct regulator_dev *rdev)
{
	arm_dummy_enable = 1;
	return 0;
}

static int armreg_disable(struct regulator_dev *rdev)
{
	arm_dummy_enable = 0;
	return 0;
}

static int armreg_is_enable(struct regulator_dev *rdev)
{
	return arm_dummy_enable;
}

static int socreg_enable(struct regulator_dev *rdev)
{
	soc_dummy_enable = 1;
	return 0;
}

static int socreg_disable(struct regulator_dev *rdev)
{
	soc_dummy_enable = 0;
	return 0;
}

static int socreg_is_enable(struct regulator_dev *rdev)
{
	return soc_dummy_enable;
}

static struct regulator_ops arm_dummy_ops = {
	.set_voltage = reg_set_voltage,
	.enable	= armreg_enable,
	.disable = armreg_disable,
	.is_enabled = armreg_is_enable,
	.list_voltage = reg_list_voltage,
};

static struct regulator_ops soc_dummy_ops = {
	.set_voltage = reg_set_voltage,
	.enable	= socreg_enable,
	.disable = socreg_disable,
	.is_enabled = socreg_is_enable,
	.list_voltage = reg_list_voltage,
};

static struct regulator_desc reg_dummy_desc = {
	.id = -1,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
};

static int reg_dummy_probe(struct platform_device *pdev)
{
	struct regulator_config config = { };
	int ret;

	if (of_device_is_compatible(pdev->dev.of_node,
				"iw,imx6-dummy-armreg")) {
		config.dev = &pdev->dev;
		config.of_node = pdev->dev.of_node;
		config.init_data = &dummy_initdata;
		reg_dummy_desc.name = "armreg-dummy";
		reg_dummy_desc.ops = &arm_dummy_ops;

		arm_dummy_regulator_rdev = regulator_register(&reg_dummy_desc, &config);
		if (IS_ERR(arm_dummy_regulator_rdev)) {
			ret = PTR_ERR(arm_dummy_regulator_rdev);
			dev_err(&pdev->dev, "Failed to register regulator: %d\n", ret);
			return ret;
		}
	}

	if (of_device_is_compatible(pdev->dev.of_node,
				"iw,imx6-dummy-socreg")) {
		config.dev = &pdev->dev;
		config.of_node = pdev->dev.of_node;
		config.init_data = &dummy_initdata;
		reg_dummy_desc.name = "socreg-dummy";
		reg_dummy_desc.ops = &soc_dummy_ops;

		soc_dummy_regulator_rdev = regulator_register(&reg_dummy_desc, &config);
		if (IS_ERR(soc_dummy_regulator_rdev)) {
			ret = PTR_ERR(soc_dummy_regulator_rdev);
			dev_err(&pdev->dev, "Failed to register regulator: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static const struct of_device_id arm_regdummy_ids[] = {
	{ .compatible = "iw,imx6-dummy-armreg" },
};

static const struct of_device_id soc_regdummy_ids[] = {
	{ .compatible = "iw,imx6-dummy-socreg" },
};
MODULE_DEVICE_TABLE(of, soc_regdummy_ids);
MODULE_DEVICE_TABLE(of, arm_regdummy_ids);

static struct platform_driver arm_regdummy_driver = {
	.probe	= reg_dummy_probe,
	.driver	= {
		.name	= "armreg-dummy",
		.owner	= THIS_MODULE,
		.of_match_table = arm_regdummy_ids,
	},
};

static struct platform_driver soc_regdummy_driver = {
	.probe	= reg_dummy_probe,
	.driver	= {
		.name	= "socreg-dummy",
		.owner	= THIS_MODULE,
		.of_match_table = soc_regdummy_ids,
	},
};
module_platform_driver(arm_regdummy_driver);
module_platform_driver(soc_regdummy_driver);

MODULE_AUTHOR("iWave Systems Technologies Pvt.Ltd");
MODULE_DESCRIPTION("iWave CPU dummy regulator Driver");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL v2");
