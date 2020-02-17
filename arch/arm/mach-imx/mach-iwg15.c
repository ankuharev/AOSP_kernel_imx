/*
 * Copyright (c) 2014-2015 iWave Systems Technologies Pvt. Ltd.
 * Copyright 2011 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/can/platform/flexcan.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clocksource.h>
#include <linux/cpu.h>
#include <linux/export.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/opp.h>
#include <linux/phy.h>
#include <linux/regmap.h>
#include <linux/cpumask.h>
#include <linux/micrel_phy.h>
#include <linux/mfd/syscon.h>
#include <linux/mfd/syscon/imx6q-iomuxc-gpr.h>
#include <linux/of_net.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/system_misc.h>
#include <linux/memblock.h>
#include <asm/setup.h>

#include "common.h"
#include "cpuidle.h"
#include "hardware.h"

#define HW_OCOTP_CFGn(n)                        (0x00000410 + (n) * 0x10)            
#define BSP_VERSION_Q7                          "iW-PRDVZ-SC-01-R5.0-REL1.0-Android5.0.0"
#define BSP_VERSION_PICO                        "iW-PREBB-SC-01-R2.0-REL1.0-Android5.0.0"
#define BSP_VERSION_MXM                         "iW-EMDO9-SC-01-R3.0-REL1.0-Android5.0.0"
#define BSP_VERSION_SM                          "iW-PREPZ-SC-01-R2.0-REL1.0-Android5.0.0"
#define SOM_REV                         1
#define BOM_REV                         2

static struct flexcan_platform_data flexcan_pdata[2];
static int flexcan0_en_gpio;
static int flexcan1_en_gpio;
static int sata_act_gpio;

static void __init imx6_iwg15_common_reset (void)
{
        struct device_node *np;
        int com_rst_gpio;

        if (of_machine_is_compatible("iw,qd_iwg15m_sm") ||
                        of_machine_is_compatible("iw,dls_iwg15m_sm"))
                np = of_find_compatible_node(NULL, NULL, "iwave,g15-sm-com");
	else if (of_machine_is_compatible("iw,qd_iwg15m_mxm") || 
	    of_machine_is_compatible("iw,dls_iwg15m_mxm")) 
	np = of_find_compatible_node(NULL, NULL, "iwave,g15-mxm-com");
	else if (of_machine_is_compatible("iw,qd_iwg15s_pico") || 
	    of_machine_is_compatible("iw,dls_iwg15s_pico")) 
		np = of_find_compatible_node(NULL, NULL, "iwave,g15-pico-com");
	else
		np = of_find_compatible_node(NULL, NULL, "iwave,g15-q7-com");
        if (!np) {
			pr_warn("failed to find g15-com node\n");
                goto put_node;
        }

        com_rst_gpio = of_get_named_gpio(np, "reset-gpios", 0);
        if (gpio_is_valid(com_rst_gpio) &&
                        !gpio_request_one(com_rst_gpio, GPIOF_DIR_OUT, "comm-rst")) {

                gpio_set_value(com_rst_gpio, 1);
                mdelay(100);
                gpio_set_value(com_rst_gpio, 0);
                mdelay(100);
                gpio_set_value(com_rst_gpio, 1);
        }
put_node:
        of_node_put(np);
}

static void __init imx6_iwg15_enet_reset (void)
{
	struct device_node *np;
	int eth_rst_gpio;

	np = of_find_compatible_node(NULL, NULL, "iwave,g15-pico-eth-reset");
	if (!np) {
		pr_warn("failed to find g15-pico-eth-reset node\n");
		goto put_node;
	}

	eth_rst_gpio = of_get_named_gpio(np, "reset-gpios", 0);
	if (gpio_is_valid(eth_rst_gpio) &&
			!gpio_request_one(eth_rst_gpio, GPIOF_DIR_OUT, "eth-rst")) {

		gpio_set_value(eth_rst_gpio, 1);
		mdelay(20);
		gpio_set_value(eth_rst_gpio, 0);
		mdelay(20);
		gpio_set_value(eth_rst_gpio, 1);
	}
put_node:
	of_node_put(np);
}

static int __init q7_som_revision (void)
{
        struct device_node *np;
        int i,val,err,num_ctrl;
        unsigned *ctrl;
        short revision = 0;

        np = of_find_compatible_node(NULL, NULL, "iwave,g15-q7-com");
        if (!np) {
                pr_warn("failed to find g15-q7-com node\n");
                revision =-1;
                goto put_node;
        }
	/* Fill GPIO pin array */
        num_ctrl = of_gpio_named_count(np, "som-rev-gpios");
        if (num_ctrl <= 0) {
                pr_warn("gpios DT property empty / missing\n");
                revision =-1;
                goto put_node;
        }

        ctrl = kzalloc(num_ctrl * sizeof(unsigned), GFP_KERNEL);
        if (!ctrl) {
                pr_warn("unable to allocate the memory\n");
                revision =-1;
                goto put_node;
        }
        for (i = 0; i < num_ctrl; i++) {

                val = of_get_named_gpio(np, "som-rev-gpios",i);
                if (val < 0) {
                        pr_warn("unable to get the gpio\n");
                        revision =-1;
                        goto put_node;
                }

                ctrl[i] = val;
        }

        /* Request as a input GPIO and read the value */
        for (i = 0; i < num_ctrl; i++) {
                err = gpio_request(ctrl[i],"som-rev GPIO");
                if (err){
                        pr_warn("unable to request for gpio\n");
                        revision =-1;
                        goto put_node;
                }

                err = gpio_direction_input(ctrl[i]);
                if (err) {
                        pr_warn("unable to request for gpio\n");
                        revision =-1;
                        goto put_node;
 	}
                revision |= gpio_get_value(ctrl[i]) << i;
        }
put_node:
        of_node_put(np);
        return revision;
}

static int __init sm_som_revision (int ver)
{
        struct device_node *np;
        int i,val,err,num_ctrl;
        unsigned *ctrl;
        short revision = 0;

        np = of_find_compatible_node(NULL, NULL, "iwave,g15-sm-com");
        if (!np) {
                pr_warn("failed to find g15-sm-com node\n");
                revision =-1;
                goto put_node;
        }

        /* Fill GPIO pin array */
	if(ver == SOM_REV)
        num_ctrl = of_gpio_named_count(np, "som-rev-gpios");
	else if(ver == BOM_REV)
        num_ctrl = of_gpio_named_count(np, "bom-rev-gpios");
        if (num_ctrl <= 0) {
                pr_warn("gpios DT property empty / missing\n");
                revision =-1;
                goto put_node;
        }

        ctrl = kzalloc(num_ctrl * sizeof(unsigned), GFP_KERNEL);
        if (!ctrl) {
                pr_warn("unable to allocate the memory\n");
                revision =-1;
                goto put_node;
        }
        for (i = 0; i < num_ctrl; i++) {
        if(ver == SOM_REV)
                val = of_get_named_gpio(np, "som-rev-gpios",i);
        else if(ver == BOM_REV)
                val = of_get_named_gpio(np, "bom-rev-gpios",i);

               if (val < 0) {
                        pr_warn("unable to get the gpio - %d\n", i);
                        revision =-1;
                        goto put_node;
                }

                ctrl[i] = val;
        }
        /* Request as a input GPIO and read the value */
        for (i = 0; i < num_ctrl; i++) {
	   if(ver == SOM_REV)
                err = gpio_request(ctrl[i],"som-rev-gpios");
	   else if(ver == BOM_REV )
                err = gpio_request(ctrl[i],"bom-rev-gpios");
                if (err){
                        pr_warn("unable to request for gpio - %d\n", i);
                        revision =-1;
                        goto put_node;
                }

                err = gpio_direction_input(ctrl[i]);
                if (err) {
                        pr_warn("unable to set gpio as input - %d\n", i);
                        revision =-1;
                        goto put_node;
                }
                revision |= gpio_get_value(ctrl[i]) << i;
        }
put_node:
        of_node_put(np);
        return revision;
}

static int __init print_board_info (void)
{
        struct device_node *np;
        unsigned int unique_id1, unique_id2;
        void __iomem *base;

        np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-ocotp");
        if (!np) {
                pr_warn("failed to find ocotp node\n");
                return 0;
        }

        base = of_iomap(np, 0);
        if (!base) {
                pr_warn("failed to map ocotp\n");
                goto put_node;
        }

        unique_id1 = readl_relaxed(base + HW_OCOTP_CFGn(0));
        unique_id2 = readl_relaxed(base + HW_OCOTP_CFGn(1));

        printk ("\n");
        printk ("Board Info:\n");
	
#ifdef CONFIG_IWG15M_Q7
        if (of_machine_is_compatible("iw,qd_iwg15m_q7") ||
                        of_machine_is_compatible("iw,dls_iwg15m_q7")) {
                printk ("\tSOM Version     : iW-PRDVZ-AP-01-R6.x\n");
        	printk ("\tBSP Version     : %s\n", BSP_VERSION_Q7);
	}
        else if (of_machine_is_compatible("iw,qd_iwg15m_q7_pmic") ||
                        of_machine_is_compatible("iw,dls_iwg15m_q7_pmic")) {
                printk ("\tSOM Version     : iW-PRDVZ-AP-01-R5.%x\n",q7_som_revision());
        	printk ("\tBSP Version     : %s\n", BSP_VERSION_Q7);
	}
#endif
#ifdef CONFIG_IWG15S_PICO
	printk ("\tSBC Version     : iW-PREBB-AP-01-R2.x\n");
       	printk ("\tBSP Version     : %s\n", BSP_VERSION_PICO);
#elif defined(CONFIG_IWG15M_MXM)
		printk ("\tSOM Version     : iW-EMDO9-AP-01-R3.x\n");
        	printk ("\tBSP Version     : %s\n", BSP_VERSION_MXM);
#endif
#ifdef CONFIG_IWG15M_SM
	printk ("\tSOM version     : iW-PREPZ-AP-01-R%1x.%1x\n",sm_som_revision(SOM_REV)+1,sm_som_revision(BOM_REV));
      	printk ("\tBSP Version     : %s\n", BSP_VERSION_SM);
#endif
        printk ("\tCPU Unique ID   : 0x%08x%08x \n", unique_id2, unique_id1);
        printk ("\n");

        iounmap(base);
put_node:
        of_node_put(np);
        return 0;
}

static void imx6_iwg15_flexcan0_switch_auto(int enable)
{
        /* Active low enables the CAN tranceiver */
        if (enable)
                gpio_set_value_cansleep(flexcan0_en_gpio, 0);
        else
                gpio_set_value_cansleep(flexcan0_en_gpio, 1);
}

static void imx6_iwg15_flexcan1_switch_auto(int enable)
{
        /* Active low enables the CAN tranceiver */
        if (enable)
                gpio_set_value_cansleep(flexcan1_en_gpio, 0);
        else
                gpio_set_value_cansleep(flexcan1_en_gpio, 1);
}

static int __init imx6_iwg15_flexcan_fixup_auto(void)
{
	struct device_node *can0,*can1;
	can0 = of_find_node_by_path("/soc/aips-bus@02000000/can@02090000");
        if (!can0)
		return -ENODEV;

	flexcan0_en_gpio = of_get_named_gpio(can0, "trx-en-gpio", 0);
        if (gpio_is_valid(flexcan0_en_gpio) &&
                        !gpio_request_one(flexcan0_en_gpio, GPIOF_DIR_OUT, "flexcan0-trx-en")) {
                flexcan_pdata[0].transceiver_switch = imx6_iwg15_flexcan0_switch_auto;
        }

        if (of_machine_is_compatible("iw,dls_iwg15m_q7") ||
			of_machine_is_compatible("iw,qd_iwg15m_q7") ||
			of_machine_is_compatible("iw,qd_iwg15s_pico") ||
			of_machine_is_compatible("iw,dls_iwg15s_pico")) {
                can1 = of_find_node_by_path("/soc/aips-bus@02000000/can@02094000");
                if (!can1)
                        return -ENODEV;

                flexcan1_en_gpio = of_get_named_gpio(can1, "trx-en-gpio", 0);
                if (gpio_is_valid(flexcan1_en_gpio) &&
                                !gpio_request_one(flexcan1_en_gpio, GPIOF_DIR_OUT, "flexcan1-trx-en")) {
                        flexcan_pdata[1].transceiver_switch = imx6_iwg15_flexcan1_switch_auto;
                        of_node_put(can1);
                }
        }

        return 0;
}

static void __init imx6_iwg15_sata_act_led (void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-ahci");
	if (!np) {
		pr_warn("failed to find fsl,imx6q-ahci node\n");
		goto put_node;
	}

	sata_act_gpio = of_get_named_gpio(np, "sata-act-gpios", 0);
	if (gpio_is_valid(sata_act_gpio) &&
			!gpio_request_one(sata_act_gpio, GPIOF_DIR_OUT, "sata-act")) {

		gpio_set_value(sata_act_gpio, 1);
	}
put_node:
	of_node_put(np);
}

void imx6_iwg15m_sata_act_led_flip(int value)
{
	if (gpio_is_valid(sata_act_gpio))
		gpio_set_value(sata_act_gpio, value);
}

static int ksz9021_phy_fixup(struct phy_device *phydev)
{
	if (IS_BUILTIN(CONFIG_PHYLIB)) {
		/* prefer master mode, 1000 Base-T capable */
		phy_write(phydev, 0x9, 0x1f00);

		/* min rx data delay */
		phy_write(phydev, 0x0b, 0x8105);
		phy_write(phydev, 0x0c, 0x0000);

		/* max rx/tx clock delay, min rx/tx control delay */
		phy_write(phydev, 0x0b, 0x8104);
		phy_write(phydev, 0x0c, 0xf0f0);
		phy_write(phydev, 0x0b, 0x104);
	}

	return 0;
}

static void mmd_write_reg(struct phy_device *dev, int device, int reg, int val)
{
	phy_write(dev, 0x0d, device);
	phy_write(dev, 0x0e, reg);
	phy_write(dev, 0x0d, (1 << 14) | device);
	phy_write(dev, 0x0e, val);
}

static int ksz9031_phy_fixup(struct phy_device *phydev)
{
        /* prefer master mode, 1000 Base-T capable */
        phy_write(phydev, 0x9, 0x1f00);

        /*
         * min rx data delay, max rx/tx clock delay,
         * min rx/tx control delay
         */
        if (of_machine_is_compatible("iw,dls_iwg15m_q7") ||
                        of_machine_is_compatible("iw,qd_iwg15m_q7")) {
                mmd_write_reg(phydev, 2, 4, 0x00c7);
                mmd_write_reg(phydev, 2, 5, 0xbb5a);
                mmd_write_reg(phydev, 2, 6, 0x7777);
	        mmd_write_reg(phydev, 2, 8, 0x039c);
	}else if (of_machine_is_compatible("iw,dls_iwg15m_q7_pmic") ||
			of_machine_is_compatible("iw,qd_iwg15m_q7_pmic") ||
			of_machine_is_compatible("iw,dls_iwg15s_pico") ||
			of_machine_is_compatible("iw,qd_iwg15s_pico") || 
			of_machine_is_compatible("iw,dls_iwg15m_mxm") ||
			of_machine_is_compatible("iw,qd_iwg15m_mxm") ||
                        of_machine_is_compatible("iw,dls_iwg15m_sm") ||
                        of_machine_is_compatible("iw,qd_iwg15m_sm")) {
                mmd_write_reg(phydev, 2, 4, 0x80);
                mmd_write_reg(phydev, 2, 5, 0x7787);
                mmd_write_reg(phydev, 2, 6, 0);
                mmd_write_reg(phydev, 2, 8, 0x03ff);
        }

	return 0;
}

static void __init imx6_iwg15_enet_phy_init(void)
{
	if (IS_BUILTIN(CONFIG_PHYLIB)) {
		phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
				ksz9021_phy_fixup);
		phy_register_fixup_for_uid(PHY_ID_KSZ9031, MICREL_PHY_ID_MASK,
				ksz9031_phy_fixup);
	}
}

static void __init imx6_iwg15_1588_init(void)
{
	struct regmap *gpr;

	gpr = syscon_regmap_lookup_by_compatible("fsl,imx6q-iomuxc-gpr");
	if (!IS_ERR(gpr))
		regmap_update_bits(gpr, IOMUXC_GPR1,
				IMX6Q_GPR1_ENET_CLK_SEL_MASK,
				IMX6Q_GPR1_ENET_CLK_SEL_ANATOP);
	else
		pr_err("failed to find fsl,imx6q-iomux-gpr regmap\n");

}

static void __init imx6_iwg15_csi_mux_init(void)
{
	 /* For MX6Q:
         * GPR1 bit19 and bit20 meaning:
         * Bit19:       0 - Enable mipi to IPU1 CSI0
         *                      virtual channel is fixed to 0
         *              1 - Enable parallel interface to IPU1 CSI0
         * Bit20:       0 - Enable mipi to IPU2 CSI1
         *                      virtual channel is fixed to 3
         *              1 - Enable parallel interface to IPU2 CSI1
         * IPU1 CSI1 directly connect to mipi csi2,
         *      virtual channel is fixed to 1
         * IPU2 CSI0 directly connect to mipi csi2,
         *      virtual channel is fixed to 2
         *      
         * For MX6DL:
         * GPR13 bit 0-2 IPU_CSI0_MUX
         *   000 MIPI_CSI0
         * GPR13 bit 3-5 IPU_CSI1_MUX
         *   100 IPU CSI1
         */

	struct regmap *gpr;

	gpr = syscon_regmap_lookup_by_compatible("fsl,imx6q-iomuxc-gpr");
	if (!IS_ERR(gpr)) {
		if (of_machine_is_compatible("iw,qd_iwg15m_q7") ||
			of_machine_is_compatible("iw,qd_iwg15m_q7_pmic")  || 
			of_machine_is_compatible("iw,qd_iwg15s_pico")) {
                        regmap_update_bits(gpr, IOMUXC_GPR1, 1 << 19, 0 << 19);
                        regmap_update_bits(gpr, IOMUXC_GPR1, 1 << 20, 1 << 20);
                }
		else if(of_machine_is_compatible("iw,qd_iwg15m_mxm")){
			regmap_update_bits(gpr, IOMUXC_GPR1, 1 << 20, 1 << 20);
		}
                else if (of_machine_is_compatible("iw,dls_iwg15m_q7") ||
			 of_machine_is_compatible("iw,dls_iwg15m_q7_pmic") || 
			 of_machine_is_compatible("iw,dls_iwg15s_pico") ||															  of_machine_is_compatible("iw,dls_iwg15m_mxm")){
                        regmap_update_bits(gpr, IOMUXC_GPR13, 0x3F, 0x20);
		}
	} else {
		pr_err("%s(): failed to find fsl,imx6q-iomux-gpr regmap\n",
		       __func__);
	}
}

#define OCOTP_MACn(n)	(0x00000620 + (n) * 0x10)
void __init imx6_iwg15_enet_mac_init(const char *compatible)
{
	struct device_node *ocotp_np, *enet_np, *from = NULL;
	void __iomem *base;
	struct property *newmac;
	u32 macaddr_low;
	u32 macaddr_high = 0;
	u32 macaddr1_high = 0;
	u8 *macaddr;
	int i;

	for (i = 0; i < 2; i++) {
		enet_np = of_find_compatible_node(from, NULL, compatible);
		if (!enet_np)
			return;

		from = enet_np;

		if (of_get_mac_address(enet_np))
			goto put_enet_node;

		ocotp_np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-ocotp");
		if (!ocotp_np) {
			pr_warn("failed to find ocotp node\n");
			goto put_enet_node;
		}

		base = of_iomap(ocotp_np, 0);
		if (!base) {
			pr_warn("failed to map ocotp\n");
			goto put_ocotp_node;
		}

		macaddr_low = readl_relaxed(base + OCOTP_MACn(1));
		if (i)
			macaddr1_high = readl_relaxed(base + OCOTP_MACn(2));
		else
			macaddr_high = readl_relaxed(base + OCOTP_MACn(0));

		newmac = kzalloc(sizeof(*newmac) + 6, GFP_KERNEL);
		if (!newmac)
			goto put_ocotp_node;

		newmac->value = newmac + 1;
		newmac->length = 6;
		newmac->name = kstrdup("local-mac-address", GFP_KERNEL);
		if (!newmac->name) {
			kfree(newmac);
			goto put_ocotp_node;
		}

		macaddr = newmac->value;
		if (i) {
			macaddr[5] = (macaddr_low >> 16) & 0xff;
			macaddr[4] = (macaddr_low >> 24) & 0xff;
			macaddr[3] = macaddr1_high & 0xff;
			macaddr[2] = (macaddr1_high >> 8) & 0xff;
			macaddr[1] = (macaddr1_high >> 16) & 0xff;
			macaddr[0] = (macaddr1_high >> 24) & 0xff;
		} else {
			macaddr[5] = macaddr_high & 0xff;
			macaddr[4] = (macaddr_high >> 8) & 0xff;
			macaddr[3] = (macaddr_high >> 16) & 0xff;
			macaddr[2] = (macaddr_high >> 24) & 0xff;
			macaddr[1] = macaddr_low & 0xff;
			macaddr[0] = (macaddr_low >> 8) & 0xff;
		}

		of_update_property(enet_np, newmac);
		iounmap(base);

put_ocotp_node:
	of_node_put(ocotp_np);
put_enet_node:
	of_node_put(enet_np);
	}
}

static inline void imx6_iwg15_enet_init(void)
{
	imx6_iwg15_enet_mac_init("fsl,imx6q-fec");
	imx6_iwg15_enet_phy_init();
	imx6_iwg15_1588_init();
}

/* Add auxdata to pass platform data */
static const struct of_dev_auxdata imx6_iwg15_auxdata_lookup[] __initconst = {
	OF_DEV_AUXDATA("fsl,imx6q-flexcan", 0x02090000, NULL, &flexcan_pdata[0]),
	OF_DEV_AUXDATA("fsl,imx6q-flexcan", 0x02094000, NULL, &flexcan_pdata[1]),
	{ /* sentinel */ }
};

static void __init imx6_iwg15_init_machine(void)
{
	struct device *parent;

	mxc_arch_reset_init_dt();
	parent = imx_soc_device_init();
	if (parent == NULL)
		pr_warn("failed to initialize soc device\n");

	of_platform_populate(NULL, of_default_bus_match_table,
					imx6_iwg15_auxdata_lookup, parent);
	
#ifdef CONFIG_IWG15S_PICO
	imx6_iwg15_enet_reset();
#endif
	imx6_iwg15_enet_init();
	imx_anatop_init();
	imx6_pm_init();
	imx6_iwg15_common_reset();
	if (!(of_machine_is_compatible("iw,qd_iwg15m_sm") ||
              of_machine_is_compatible("iw,dls_iwg15m_sm")))
		{
		imx6_iwg15_csi_mux_init();
		}
#ifdef CONFIG_IWG15M_Q7
       if (cpu_is_imx6q()) 
               imx6_iwg15_sata_act_led();
#endif
	print_board_info();
}

#define OCOTP_CFG3			0x440
#define OCOTP_CFG3_SPEED_SHIFT		16
#define OCOTP_CFG3_SPEED_1P2GHZ		0x3
#define OCOTP_CFG3_SPEED_1GHZ		0x2
#define OCOTP_CFG3_SPEED_850MHZ		0x1
#define OCOTP_CFG3_SPEED_800MHZ		0x0

static void __init imx6_iwg15_opp_check_speed_grading(struct device *cpu_dev)
{
	struct device_node *np;
	void __iomem *base;
	u32 val;

	np = of_find_compatible_node(NULL, NULL, "fsl,imx6q-ocotp");
	if (!np) {
		pr_warn("failed to find ocotp node\n");
		return;
	}

	base = of_iomap(np, 0);
	if (!base) {
		pr_warn("failed to map ocotp\n");
		goto put_node;
	}

	/*
	 * SPEED_GRADING[1:0] defines the max speed of ARM:
	 * 2b'11: 1200000000Hz; -- i.MX6Q only.
	 * 2b'10: 1000000000Hz;
	 * 2b'01: 850000000Hz; -- i.MX6Q Only, exclusive with 1GHz.
	 * 2b'00: 800000000Hz;
	 * We need to set the max speed of ARM according to fuse map.
	 */
	val = readl_relaxed(base + OCOTP_CFG3);
	val >>= OCOTP_CFG3_SPEED_SHIFT;
	if (cpu_is_imx6q()) {
		if ((val & 0x3) < OCOTP_CFG3_SPEED_1P2GHZ)
			if (opp_disable(cpu_dev, 1200000000))
				pr_warn("failed to disable 1.2 GHz OPP\n");
	}
	if ((val & 0x3) < OCOTP_CFG3_SPEED_1GHZ)
		if (opp_disable(cpu_dev, 996000000))
			pr_warn("failed to disable 1 GHz OPP\n");
	if (cpu_is_imx6q()) {
		if ((val & 0x3) < OCOTP_CFG3_SPEED_850MHZ ||
			(val & 0x3) == OCOTP_CFG3_SPEED_1GHZ)
			if (opp_disable(cpu_dev, 852000000))
				pr_warn("failed to disable 850 MHz OPP\n");
	}

	if (IS_ENABLED(CONFIG_MX6_VPU_352M)) {
		if (opp_disable(cpu_dev, 396000000))
			pr_warn("failed to disable 396MHz OPP\n");
		pr_info("remove 396MHz OPP for VPU running at 352MHz!\n");
	}

put_node:
	of_node_put(np);
}

static void __init imx6_iwg15_opp_init(struct device *cpu_dev)
{
	struct device_node *np;

	np = of_find_node_by_path("/cpus/cpu@0");
	if (!np) {
		pr_warn("failed to find cpu0 node\n");
		return;
	}

	cpu_dev->of_node = np;
	if (of_init_opp_table(cpu_dev)) {
		pr_warn("failed to init OPP table\n");
		goto put_node;
	}

	imx6_iwg15_opp_check_speed_grading(cpu_dev);

put_node:
	of_node_put(np);
}


static struct platform_device imx6_iwg15_cpufreq_pdev = {
	.name = "imx6-cpufreq",
};

static void __init imx6_iwg15_init_late(void)
{
	struct regmap *gpr;

	/*
	 * Need to force IOMUXC irq pending to meet CCM low power mode
	 * restriction, this is recommended by hardware team.
	 */
	gpr = syscon_regmap_lookup_by_compatible("fsl,imx6q-iomuxc-gpr");
	if (!IS_ERR(gpr))
		regmap_update_bits(gpr, IOMUXC_GPR1,
			IMX6Q_GPR1_GINT_MASK,
			IMX6Q_GPR1_GINT_ASSERT);

	/*
	 * WAIT mode is broken on TO 1.0 and 1.1, so there is no point
	 * to run cpuidle on them.
	 */
	if ((cpu_is_imx6q() && imx_get_soc_revision() > IMX_CHIP_REVISION_1_1)
		|| (cpu_is_imx6dl() && imx_get_soc_revision() >
		IMX_CHIP_REVISION_1_0))
		imx6q_cpuidle_init();

	if (IS_ENABLED(CONFIG_ARM_IMX6_CPUFREQ)) {
		imx6_iwg15_opp_init(&imx6_iwg15_cpufreq_pdev.dev);
		platform_device_register(&imx6_iwg15_cpufreq_pdev);
	}

		imx6_iwg15_flexcan_fixup_auto();
}

static void __init imx6_iwg15_map_io(void)
{
	debug_ll_io_init();
	imx_scu_map_io();
	imx6_pm_map_io();
	imx6_busfreq_map_io();
}

static void __init imx6_iwg15_init_irq(void)
{
	imx_init_revision_from_anatop();
	imx_init_l2cache();
	imx_src_init();
	imx_gpc_init();
	irqchip_init();
}

static void __init imx6_iwg15_timer_init(void)
{
	of_clk_init(NULL);
	clocksource_of_init();
	if (cpu_is_imx6dl()) {
                if (num_possible_cpus() == 2)
                        imx_print_silicon_rev("i.MX6DL",imx_get_soc_revision());
                else
                        imx_print_silicon_rev("i.MX6S",imx_get_soc_revision());
        } else if (cpu_is_imx6q()) {
                if (num_possible_cpus() == 4)
                        imx_print_silicon_rev("i.MX6Q",imx_get_soc_revision());
                else
                        imx_print_silicon_rev("i.MX6D",imx_get_soc_revision());
        }

}

static const char *imx6_iwg15_dt_compat[] __initdata = {
	"iw,qd_iwg15m_q7",
	"iw,qd_iwg15m_q7_pmic",
	"iw,dls_iwg15m_q7",
	"iw,dls_iwg15m_q7_pmic",
	"iw,qd_iwg15s_pico",
	"iw,dls_iwg15s_pico",
	"iw,qd_iwg15m_mxm",
	"iw,dls_iwg15m_mxm",
	"iw,qd_iwg15m_sm",
	"iw,dls_iwg15m_sm",
	NULL,
};

extern unsigned long int ramoops_phys_addr;
extern unsigned long int ramoops_mem_size;
static void imx6_iwg15_reserve(void)
{
	phys_addr_t phys;
	phys_addr_t max_phys;
	struct meminfo *mi;
	struct membank *bank;

#ifdef CONFIG_PSTORE_RAM
	mi = &meminfo;
	if (!mi) {
		pr_err("no memory reserve for ramoops.\n");
		return;
	}

	/* use memmory last bank for ram console store */
	bank = &mi->bank[mi->nr_banks - 1];
	if (!bank) {
		pr_err("no memory reserve for ramoops.\n");
		return;
	}
	max_phys = bank->start + bank->size;
	/* reserve 64M for uboot avoid ram console data is cleaned by uboot */
	phys = memblock_alloc_base(SZ_1M, SZ_4K, max_phys - SZ_64M);
	if (phys) {
		memblock_remove(phys, SZ_1M);
		memblock_reserve(phys, SZ_1M);
		ramoops_phys_addr = phys;
		ramoops_mem_size = SZ_1M;
	} else {
		ramoops_phys_addr = 0;
		ramoops_mem_size = 0;
		pr_err("no memory reserve for ramoops.\n");
	}
#endif
	return;
}

DT_MACHINE_START(IMX6_IWG15, "iW-RainboW-G15 platform based on i.MX6 (Device Tree)")
	/*
	 * i.MX6Q/DL maps system memory at 0x10000000 (offset 256MiB), and
	 * GPU has a limit on physical address that it accesses, which must
	 * be below 2GiB.
	 */
	.dma_zone_size	= (SZ_2G - SZ_256M),
	.smp		= smp_ops(imx_smp_ops),
	.map_io		= imx6_iwg15_map_io,
	.init_irq	= imx6_iwg15_init_irq,
	.init_time	= imx6_iwg15_timer_init,
	.init_machine	= imx6_iwg15_init_machine,
	.init_late      = imx6_iwg15_init_late,
	.dt_compat	= imx6_iwg15_dt_compat,
	.reserve	= imx6_iwg15_reserve,
	.restart	= mxc_restart,
MACHINE_END
