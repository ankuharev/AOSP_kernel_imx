/*
 * Copyright (c) 2014 iWave Systems Technologies Pvt. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 */
 
/*
 * @file dip-sw.c
 *
 * @brief Driver for Dip-switch interface
 *
 * @ingroup Dip-switch driver 
 */

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/opp.h>
#include <linux/leds.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <linux/of_net.h>
#include <asm/mach/arch.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include "dip-sw.h"


int num_ctrd;
unsigned  *ctrd;

static int dip_open(struct inode *inode, struct file *file)
{
	pr_debug("\r\n Inside: %s",__func__);

	return 0;
}

static int dip_close(struct inode *inode, struct file *file)
{
	pr_debug("\r\n Inside: %s\r\n",__func__);

	return 0;
}

static int dip_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int num_ctrd = sizeof(ctrd)/sizeof(int);
	int status,num,temp;
	
	temp = copy_from_user (&num, (int *) arg, sizeof(int));
	pr_debug("\r\n Inside: %s, cmd: %d, arg: %d\r\n",__func__, cmd, num);
	num = num -1;

	switch (cmd)
	{
		case PIN_STATUS:
			status = gpio_get_value(ctrd[num]);
			break;

		default:
			pr_debug("Unknown ioctl command %x\r\n", cmd);
			return -EINVAL;   /* Invalid argument */
	}

        copy_to_user ( (int *) arg, &status, sizeof(int));

	return status;
}

static const struct of_device_id of_gpio_dipsw_match[] = {
	{ .compatible = "gpio-dip", },
	{},
};

static const struct file_operations dip_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = dip_ioctl,
	.open  = dip_open,
	.release = dip_close,
};

static int gpio_dip_sw_probe()

{
	struct device_node *npd;
	int i,val,err;

	npd = of_find_compatible_node(NULL, NULL, "gpio-dip");

	if (!npd) {
		pr_warn("failed to find dip node\n");
	}

	/* Fill GPIO pin array */
	num_ctrd = of_gpio_named_count(npd,"dip-gpios");
	if (num_ctrd <= 0) {
		pr_warn("gpios DT property empty / missing\n");
	}

	ctrd = kzalloc(num_ctrd * sizeof(unsigned), GFP_KERNEL);
	if (!ctrd) {
		pr_warn("unable to allocate the memory\n");
	}

	for (i = 0; i < num_ctrd; i++) {
		val = of_get_named_gpio(npd, "dip-gpios",i);
		if (val < 0) {
			pr_warn("unable to get the gpio\n");
		}

		ctrd[i] = val;
	}

	/* Request as a Input GPIO  */
	for (i = 0; i < num_ctrd; i++) {
		err = gpio_request(ctrd[i],"dip-gpios");
		if (err){
			pr_warn("unable to request for gpio\n");
		}
		err = gpio_direction_input(ctrd[i]);
		if (err) {
			pr_warn("unable to request for gpio\n");
		}

	}
	return 0;
}

static struct platform_driver gpio_dip_sw_driver = {
        .probe   =gpio_dip_sw_probe,
        .driver  = {
                .name   = "dipsw",
                .owner  = THIS_MODULE,
                .of_match_table = of_match_ptr(of_gpio_dipsw_match),
        },
};

unsigned int __init dip_init(void)
{
	/* Register the GPIO character driver */
	dip_sw_major_no = register_chrdev(0,"dip_sw",&dip_fops);
	printk("Dip switch major no %d\r\n ",dip_sw_major_no);
	if(dip_sw_major_no <= 0)
	{
		printk ("%s: unable to get major number%d\r\n",
				"dip switch", dip_sw_major_no);
		return -1;
	}

	return 0;
}

/*!
 * @brief Cleanup or Deinitialization
 */
void __exit dip_deinit(void)
{
	/* Freeing the GPIO PIN's */
	gpio_free_array(ctrd,num_ctrd);

	/* Free the memory used for array */ 
	kfree(ctrd);

	/* Un-register the GPIO Character Driver */
	unregister_chrdev (dip_sw_major_no, "led");
}

module_init(dip_init);
module_platform_driver(gpio_dip_sw_driver);
module_exit(dip_deinit);
