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
 * @file led.c
 *
 * @brief Driver for status LED interface
 *
 * @ingroup Status LED driver 
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
#include "led.h"

int num_ctrl;
unsigned int led_major_no;
unsigned  *ctrl;

static int led_open(struct inode *inode, struct file *file)
{
	pr_debug("\r\n Inside: %s",__func__);

	return 0;
}

static int led_close(struct inode *inode, struct file *file)
{
	pr_debug("\r\n Inside: %s\r\n",__func__);

	return 0;
}

static int led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int num_ctrl = sizeof(ctrl)/sizeof(int);
	int num,temp;

	temp=copy_from_user (&num, (int *) arg, sizeof(int));
	num = num -1;
	pr_debug("\r\n Inside: %s, cmd: %d, arg: %d\r\n",__func__, cmd, arg);

	switch (cmd)
	{
		case LED_OFF:
			gpio_set_value(ctrl[num], 1);
			break;

		case LED_ON:
			gpio_set_value(ctrl[num], 0);
			break;

		default:
			pr_debug("Unknown ioctl command %x\r\n", cmd);
			pr_debug("%d \r\n",ctrl[num]);
			return -EINVAL;   /* Invalid argument */
	}
	return 0;
}

static const struct of_device_id of_gpio_leds_match[] = {
	{ .compatible = "gpio-leds", },
	{},
};

static const struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = led_ioctl,
	.open  = led_open,
	.release = led_close,
};

static int gpio_led_probe()
{
	struct device_node *np;
	int i,val,err;

	np = of_find_compatible_node(NULL, NULL, "gpio-leds");

	if (!np) {
		pr_warn("failed to find led node\n");
	}

	/* Fill GPIO pin array */
	num_ctrl = of_gpio_named_count(np, "led-gpios");
	if (num_ctrl <= 0) {
		pr_warn("gpios DT property empty / missing\n");
	}

	ctrl = kzalloc(num_ctrl * sizeof(unsigned), GFP_KERNEL);
	if (!ctrl) {
		pr_warn("unable to allocate the memory\n");
	}

	for (i = 0; i < num_ctrl; i++) {
		val = of_get_named_gpio(np, "led-gpios",i);
		if (val < 0) {
			pr_warn("unable to get the gpio\n");
		}

		ctrl[i] = val;
	}

	/* Request as a output GPIO  */
	for (i = 0; i < num_ctrl; i++) {
		err = gpio_request_one(ctrl[i], GPIOF_DIR_OUT, "led-gpios");
		if (err){
			pr_warn("unable to request for gpio\n");
		}
		gpio_set_value(ctrl[i], 0);

	}
	return 0;
}

static struct platform_driver gpio_led_driver = {
        .probe   =gpio_led_probe,
        .driver  = {
                .name   = "led",
                .owner  = THIS_MODULE,
                .of_match_table = of_match_ptr(of_gpio_leds_match),
        },
};

unsigned int __init led_init(void)
{
	/* Register the GPIO character driver */
	led_major_no = register_chrdev(0,"led",&led_fops);
	printk("LED major no %d\r\n ",led_major_no);
	if(led_major_no <= 0)
	{
		printk ("%s: unable to get major number%d\r\n",
				"led", led_major_no);
		return -1;
	}

	return 0;
}

/*!
 * @brief Cleanup or Deinitialization
 */
void __exit led_deinit(void)
{
	/* Freeing the GPIO PIN's */
	gpio_free_array(ctrl,num_ctrl);

	/* Free the memory used for array */ 
	kfree(ctrl);

	/* Un-register the GPIO Character Driver */
	unregister_chrdev (led_major_no, "led");
}
module_init(led_init);
module_platform_driver(gpio_led_driver);
module_exit(led_deinit);
