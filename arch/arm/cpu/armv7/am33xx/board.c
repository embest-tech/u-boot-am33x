/*
 * board.c
 *
 * Common board functions for AM33XX based boards
 *
 * Copyright (C) 2011, Texas Instruments, Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR /PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <errno.h>
#include <spl.h>
#include <asm/arch/cpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/omap.h>
#include <asm/arch/ddr_defs.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mem.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <asm/emif.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <miiphy.h>
#include <cpsw.h>
#include <asm/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/musb.h>
#include <asm/omap_musb.h>
#include "pmic.h"
#include "tps65217.h"

DECLARE_GLOBAL_DATA_PTR;

struct wd_timer *wdtimer = (struct wd_timer *)WDT_BASE;
struct uart_sys *uart_base = (struct uart_sys *)DEFAULT_UART_BASE;

static const struct gpio_bank gpio_bank_am33xx[4] = {
	{ (void *)AM33XX_GPIO0_BASE, METHOD_GPIO_24XX },
	{ (void *)AM33XX_GPIO1_BASE, METHOD_GPIO_24XX },
	{ (void *)AM33XX_GPIO2_BASE, METHOD_GPIO_24XX },
	{ (void *)AM33XX_GPIO3_BASE, METHOD_GPIO_24XX },
};

const struct gpio_bank *const omap_gpio_bank = gpio_bank_am33xx;

/* MII mode defines */
#define MII_MODE_ENABLE		0x0
#define RGMII_MODE_ENABLE	0xA

/* GPIO that controls power to DDR on EVM-SK */
#define GPIO_DDR_VTT_EN		7

static struct ctrl_dev *cdev = (struct ctrl_dev *)CTRL_DEVICE_BASE;

static struct am335x_baseboard_id __attribute__((section (".data"))) header;

static inline int board_is_bone(void)
{
	return !strncmp(header.name, "A335BONE", HDR_NAME_LEN);
}

static inline int board_is_evm_sk(void)
{
	return !strncmp("A335X_SK", header.name, HDR_NAME_LEN);
}

int board_is_evm_15_or_later(void)
{
	return (!strncmp("A33515BB", header.name, 8) &&
			strncmp("1.5", header.version, 3) <= 0);
}

/*
 * Read header information from EEPROM into global structure.
 */
static int read_eeprom(void)
{
	/* Check if baseboard eeprom is available */
	if (i2c_probe(CONFIG_SYS_I2C_EEPROM_ADDR)) {
		puts("Could not probe the EEPROM; something fundamentally "
			"wrong on the I2C bus.\n");
		return -ENODEV;
	}

	/* read the eeprom using i2c */
	if (i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, 0, 2, (uchar *)&header,
							sizeof(header))) {
		puts("Could not read the EEPROM; something fundamentally"
			" wrong on the I2C bus.\n");
		return -EIO;
	}

	if (header.magic != 0xEE3355AA) {
		/*
		 * read the eeprom using i2c again,
		 * but use only a 1 byte address
		 */
		if (i2c_read(CONFIG_SYS_I2C_EEPROM_ADDR, 0, 1,
					(uchar *)&header, sizeof(header))) {
			puts("Could not read the EEPROM; something "
				"fundamentally wrong on the I2C bus.\n");
			return -EIO;
		}

		if (header.magic != 0xEE3355AA) {
			printf("Incorrect magic number (0x%x) in EEPROM\n",
					header.magic);
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * Determine what type of DDR we have.
 */
static short inline board_memory_type(void)
{
	/* The following boards are known to use DDR3. */
	if (board_is_evm_sk() || board_is_evm_15_or_later())
		return EMIF_REG_SDRAM_TYPE_DDR3;

	return EMIF_REG_SDRAM_TYPE_DDR2;
}

/**
 * tps65217_reg_read() - Generic function that can read a TPS65217 register
 * @src_reg:          Source register address
 * @src_val:          Address of destination variable
 */

unsigned char tps65217_reg_read(uchar src_reg, uchar *src_val)
{
        if (i2c_read(TPS65217_CHIP_PM, src_reg, 1, src_val, 1))
                return 1;
        return 0;
}

/**
 *  tps65217_reg_write() - Generic function that can write a TPS65217 PMIC
 *                         register or bit field regardless of protection
 *                         level.
 *
 *  @prot_level:        Register password protection.
 *                      use PROT_LEVEL_NONE, PROT_LEVEL_1, or PROT_LEVEL_2
 *  @dest_reg:          Register address to write.
 *  @dest_val:          Value to write.
 *  @mask:              Bit mask (8 bits) to be applied.  Function will only
 *                      change bits that are set in the bit mask.
 *
 *  @return:            0 for success, 1 for failure.
 */
int tps65217_reg_write(uchar prot_level, uchar dest_reg,
        uchar dest_val, uchar mask)
{
        uchar read_val;
        uchar xor_reg;

        /* if we are affecting only a bit field, read dest_reg and apply the mask */
        if (mask != MASK_ALL_BITS) {
                if (i2c_read(TPS65217_CHIP_PM, dest_reg, 1, &read_val, 1))
                        return 1;
                read_val &= (~mask);
                read_val |= (dest_val & mask);
                dest_val = read_val;
        }

        if (prot_level > 0) {
                xor_reg = dest_reg ^ PASSWORD_UNLOCK;
                if (i2c_write(TPS65217_CHIP_PM, PASSWORD, 1, &xor_reg, 1))
                        return 1;
        }

        if (i2c_write(TPS65217_CHIP_PM, dest_reg, 1, &dest_val, 1))
                return 1;

        if (prot_level == PROT_LEVEL_2) {
                if (i2c_write(TPS65217_CHIP_PM, PASSWORD, 1, &xor_reg, 1))
                        return 1;

                if (i2c_write(TPS65217_CHIP_PM, dest_reg, 1, &dest_val, 1))
                        return 1;
        }

        return 0;
}

int tps65217_voltage_update(unsigned char dc_cntrl_reg, unsigned char volt_sel)
{
        if ((dc_cntrl_reg != DEFDCDC1) && (dc_cntrl_reg != DEFDCDC2)
                && (dc_cntrl_reg != DEFDCDC3))
                return 1;

        /* set voltage level */
        if (tps65217_reg_write(PROT_LEVEL_2, dc_cntrl_reg, volt_sel, MASK_ALL_BITS))
                return 1;

        /* set GO bit to initiate voltage transition */
        if (tps65217_reg_write(PROT_LEVEL_2, DEFSLEW, DCDC_GO, DCDC_GO))
                return 1;

        return 0;
}

/*
 * voltage switching for MPU frequency switching.
 * @module = mpu - 0, core - 1
 * @vddx_op_vol_sel = vdd voltage to set
 */

#define MPU     0
#define CORE    1

int voltage_update(unsigned int module, unsigned char vddx_op_vol_sel)
{
        uchar buf[4];
        unsigned int reg_offset;

        if(module == MPU)
                reg_offset = PMIC_VDD1_OP_REG;
        else
                reg_offset = PMIC_VDD2_OP_REG;

        /* Select VDDx OP   */
        if (i2c_read(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
                return 1;

        buf[0] &= ~PMIC_OP_REG_CMD_MASK;

        if (i2c_write(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
                return 1;

        /* Configure VDDx OP  Voltage */
        if (i2c_read(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
                return 1;

        buf[0] &= ~PMIC_OP_REG_SEL_MASK;
        buf[0] |= vddx_op_vol_sel;

        if (i2c_write(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
                return 1;

        if (i2c_read(PMIC_CTRL_I2C_ADDR, reg_offset, 1, buf, 1))
                return 1;

        if ((buf[0] & PMIC_OP_REG_SEL_MASK ) != vddx_op_vol_sel)
                return 1;

        return 0;
}

/* UART Defines */
#ifdef CONFIG_SPL_BUILD
#define UART_RESET		(0x1 << 1)
#define UART_CLK_RUNNING_MASK	0x1
#define UART_SMART_IDLE_EN	(0x1 << 0x3)

void am33xx_spl_board_init(void)
{
	if (!strncmp("A335BONE", header.name, 8)) {
		/* BeagleBone PMIC Code */
		uchar pmic_status_reg;

		if (i2c_probe(TPS65217_CHIP_PM))
			return;

		if (tps65217_reg_read(STATUS, &pmic_status_reg))
			return;

		/* Increase USB current limit to 1300mA */
		if (tps65217_reg_write(PROT_LEVEL_NONE, POWER_PATH,
				       USB_INPUT_CUR_LIMIT_1300MA,
				       USB_INPUT_CUR_LIMIT_MASK))
			printf("tps65217_reg_write failure\n");

		/* Only perform PMIC configurations if board rev > A1 */
		if (!strncmp(header.version, "00A1", 4))
			return;

		/* Set DCDC2 (MPU) voltage to 1.275V */
		if (tps65217_voltage_update(DEFDCDC2,
					     DCDC_VOLT_SEL_1275MV)) {
			printf("tps65217_voltage_update failure\n");
			return;
		}

		/* Set LDO3, LDO4 output voltage to 3.3V */
		if (tps65217_reg_write(PROT_LEVEL_2, DEFLS1,
				       LDO_VOLTAGE_OUT_3_3, LDO_MASK))
			printf("tps65217_reg_write failure\n");

		if (tps65217_reg_write(PROT_LEVEL_2, DEFLS2,
				       LDO_VOLTAGE_OUT_3_3, LDO_MASK))
			printf("tps65217_reg_write failure\n");

		if (!(pmic_status_reg & PWR_SRC_AC_BITMASK)) {
			printf("No AC power, disabling frequency switch\n");
			return;
		}

		/* Set MPU Frequency to 720MHz */
		mpu_pll_config(MPUPLL_M_720);
	} else {
		uchar buf[4];
		/*
		 * EVM PMIC code.  All boards currently want an MPU voltage
		 * of 1.2625V and CORE voltage of 1.1375V to operate at
		 * 720MHz.
		 */
		if (i2c_probe(PMIC_CTRL_I2C_ADDR))
			return;

		/* VDD1/2 voltage selection register access by control i/f */
		if (i2c_read(PMIC_CTRL_I2C_ADDR, PMIC_DEVCTRL_REG, 1, buf, 1))
			return;

		buf[0] |= PMIC_DEVCTRL_REG_SR_CTL_I2C_SEL_CTL_I2C;

		if (i2c_write(PMIC_CTRL_I2C_ADDR, PMIC_DEVCTRL_REG, 1, buf, 1))
			return;

		if (!voltage_update(MPU, PMIC_OP_REG_SEL_1_2_6) &&
				!voltage_update(CORE, PMIC_OP_REG_SEL_1_1_3))
			/* Frequency switching for OPP 120 */
	 		mpu_pll_config(MPUPLL_M_720);
	}
}
#endif

/*
 * early system init of muxing and clocks.
 */
void s_init(void)
{
	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	writel(0xAAAA, &wdtimer->wdtwspr);
	while (readl(&wdtimer->wdtwwps) != 0x0)
		;
	writel(0x5555, &wdtimer->wdtwspr);
	while (readl(&wdtimer->wdtwwps) != 0x0)
		;

#ifdef CONFIG_SPL_BUILD
	/* Setup the PLLs and the clocks for the peripherals */
	pll_init();

	/* UART softreset */
	u32 regVal;

	enable_uart0_pin_mux();

	regVal = readl(&uart_base->uartsyscfg);
	regVal |= UART_RESET;
	writel(regVal, &uart_base->uartsyscfg);
	while ((readl(&uart_base->uartsyssts) &
		UART_CLK_RUNNING_MASK) != UART_CLK_RUNNING_MASK)
		;

	/* Disable smart idle */
	regVal = readl(&uart_base->uartsyscfg);
	regVal |= UART_SMART_IDLE_EN;
	writel(regVal, &uart_base->uartsyscfg);

	gd = &gdata;

	preloader_console_init();

	/* Initalize the board header */
	enable_i2c0_pin_mux();
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	if (read_eeprom() < 0)
		puts("Could not get board ID.\n");

	enable_board_pin_mux(&header);
	if (board_is_evm_sk()) {
		/*
		 * EVM SK 1.2A and later use gpio0_7 to enable DDR3.
		 * This is safe enough to do on older revs.
		 */
		gpio_request(GPIO_DDR_VTT_EN, "ddr_vtt_en");
		gpio_direction_output(GPIO_DDR_VTT_EN, 1);
	}

	config_ddr(board_memory_type());
#endif
}

#if defined(CONFIG_OMAP_HSMMC) && !defined(CONFIG_SPL_BUILD)
int board_mmc_init(bd_t *bis)
{
	int ret;
	
	ret = omap_mmc_init(0, 0, 0);
	if (ret)
		return ret;

	return omap_mmc_init(1, 0, 0);
}
#endif

void setup_clocks_for_console(void)
{
	/* Not yet implemented */
	return;
}

/* AM33XX has two MUSB controllers which can be host or gadget */
#if (defined(CONFIG_MUSB_GADGET) || defined(CONFIG_MUSB_HOST)) && \
	(defined(CONFIG_AM335X_USB0) || defined(CONFIG_AM335X_USB1))
/* USB 2.0 PHY Control */
#define CM_PHY_PWRDN			(1 << 0)
#define CM_PHY_OTG_PWRDN		(1 << 1)
#define OTGVDET_EN			(1 << 19)
#define OTGSESSENDEN			(1 << 20)

static void am33xx_usb_set_phy_power(u8 on, u32 *reg_addr)
{
	if (on) {
		clrsetbits_le32(reg_addr, CM_PHY_PWRDN | CM_PHY_OTG_PWRDN,
				OTGVDET_EN | OTGSESSENDEN);
	} else {
		clrsetbits_le32(reg_addr, 0, CM_PHY_PWRDN | CM_PHY_OTG_PWRDN);
	}
}

static struct musb_hdrc_config musb_config = {
	.multipoint     = 1,
	.dyn_fifo       = 1,
	.num_eps        = 16,
	.ram_bits       = 12,
};

#ifdef CONFIG_AM335X_USB0
static void am33xx_otg0_set_phy_power(u8 on)
{
	am33xx_usb_set_phy_power(on, &cdev->usb_ctrl0);
}

struct omap_musb_board_data otg0_board_data = {
	.set_phy_power = am33xx_otg0_set_phy_power,
};

static struct musb_hdrc_platform_data otg0_plat = {
	.mode           = CONFIG_AM335X_USB0_MODE,
	.config         = &musb_config,
	.power          = 50,
	.platform_ops	= &musb_dsps_ops,
	.board_data	= &otg0_board_data,
};
#endif

#ifdef CONFIG_AM335X_USB1
static void am33xx_otg1_set_phy_power(u8 on)
{
	am33xx_usb_set_phy_power(on, &cdev->usb_ctrl1);
}

struct omap_musb_board_data otg1_board_data = {
	.set_phy_power = am33xx_otg1_set_phy_power,
};

static struct musb_hdrc_platform_data otg1_plat = {
	.mode           = CONFIG_AM335X_USB1_MODE,
	.config         = &musb_config,
	.power          = 50,
	.platform_ops	= &musb_dsps_ops,
	.board_data	= &otg1_board_data,
};
#endif
#endif

int arch_misc_init(void)
{
#ifdef CONFIG_AM335X_USB0
	musb_register(&otg0_plat, &otg0_board_data,
		(void *)AM335X_USB0_OTG_BASE);
#endif
#ifdef CONFIG_AM335X_USB1
	musb_register(&otg1_plat, &otg1_board_data,
		(void *)AM335X_USB1_OTG_BASE);
#endif
	return 0;
}

/*
 * Basic board specific setup.  Pinmux has been handled already.
 */
int board_init(void)
{
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	if (read_eeprom() < 0)
		puts("Could not get board ID.\n");

	gd->bd->bi_boot_params = PHYS_DRAM_1 + 0x100;

	gpmc_init();

	return 0;
}

#if (defined(CONFIG_DRIVER_TI_CPSW) && !defined(CONFIG_SPL_BUILD)) || \
	(defined(CONFIG_SPL_ETH_SUPPORT) && defined(CONFIG_SPL_BUILD))
static void cpsw_control(int enabled)
{
	/* VTP can be added here */

	return;
}

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.slave_reg_ofs	= 0x208,
		.sliver_reg_ofs	= 0xd80,
		.phy_id		= 0,
	},
	{
		.slave_reg_ofs	= 0x308,
		.sliver_reg_ofs	= 0xdc0,
		.phy_id		= 1,
	},
};

static struct cpsw_platform_data cpsw_data = {
	.mdio_base		= AM335X_CPSW_MDIO_BASE,
	.cpsw_base		= AM335X_CPSW_BASE,
	.mdio_div		= 0xff,
	.channels		= 8,
	.cpdma_reg_ofs		= 0x800,
	.slaves			= 1,
	.slave_data		= cpsw_slaves,
	.ale_reg_ofs		= 0xd00,
	.ale_entries		= 1024,
	.host_port_reg_ofs	= 0x108,
	.hw_stats_reg_ofs	= 0x900,
	.mac_control		= (1 << 5),
	.control		= cpsw_control,
	.host_port_num		= 0,
	.version		= CPSW_CTRL_VERSION_2,
};
#endif

#if defined(CONFIG_DRIVER_TI_CPSW) || \
	(defined(CONFIG_USB_ETHER) && defined(CONFIG_MUSB_GADGET))
int board_eth_init(bd_t *bis)
{
	int rv, ret = 0;
	uint8_t mac_addr[6];
	uint32_t mac_hi, mac_lo;

	/* try reading mac address from efuse */
	mac_lo = readl(&cdev->macid0l);
	mac_hi = readl(&cdev->macid0h);
	mac_addr[0] = mac_hi & 0xFF;
	mac_addr[1] = (mac_hi & 0xFF00) >> 8;
	mac_addr[2] = (mac_hi & 0xFF0000) >> 16;
	mac_addr[3] = (mac_hi & 0xFF000000) >> 24;
	mac_addr[4] = mac_lo & 0xFF;
	mac_addr[5] = (mac_lo & 0xFF00) >> 8;

#if (defined(CONFIG_DRIVER_TI_CPSW) && !defined(CONFIG_SPL_BUILD)) || \
	(defined(CONFIG_SPL_ETH_SUPPORT) && defined(CONFIG_SPL_BUILD))
	if (!getenv("ethaddr")) {
		debug("<ethaddr> not set. Reading from E-fuse\n");

		if (!is_valid_ether_addr(mac_addr)) {
			u_int32_t i;
			debug("Did not find a valid mac address in e-fuse. "
					"Trying the one present in EEPROM\n");

			for (i = 0; i < HDR_ETH_ALEN; i++)
				mac_addr[i] = header.mac_addr[0][i];
		}

		if (is_valid_ether_addr(mac_addr))
			eth_setenv_enetaddr("ethaddr", mac_addr);
		else {
			printf("Caution: Using hardcoded mac address. "
				"Set <ethaddr> variable to overcome this.\n");
		}
	}

	if (board_is_bone()) {
		writel(MII_MODE_ENABLE, &cdev->miisel);
		cpsw_slaves[0].phy_if = cpsw_slaves[1].phy_if =
				PHY_INTERFACE_MODE_MII;
	} else {
		writel(RGMII_MODE_ENABLE, &cdev->miisel);
		cpsw_slaves[0].phy_if = cpsw_slaves[1].phy_if =
				PHY_INTERFACE_MODE_RGMII;
	}

	rv = cpsw_register(&cpsw_data);
	if (rv < 0) {
		ret = -1;
		printf("Error %d registering CPSW switch\n", rv);
	}
#endif
#if defined(CONFIG_USB_ETHER) && \
	(!defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_USB_ETH_SUPPORT))
	if (is_valid_ether_addr(mac_addr))
		eth_setenv_enetaddr("usbnet_devaddr", mac_addr);
	/*
	 * SPL does not call arch_misc_init and the ROM only supports using
	 * USB0 for boot.
	 */
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_USB_ETH_SUPPORT)
	rv = musb_register(&otg0_plat, &otg0_board_data,
			(void *)AM335X_USB0_OTG_BASE);
	if (rv < 0) {
		printf("Error %d registering MUSB device\n", rv);
		return -1;
	}
#endif
	rv = usb_eth_initialize(bis);
	if (rv < 0) {
		printf("Error %d registering USB_ETHER\n", rv);
		return -1;
	}
#endif
	return ret;
}
#endif
